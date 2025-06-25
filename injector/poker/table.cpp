#include "table.hpp"
#include "../hooks/websocket.hpp"
#include <random>
#include <thread>
#include <chrono>
#include "../utils/config.hpp"
#include "../utils/utils.hpp"
#include "../protocol/game_message/take_seat.hpp"
#include "../protocol/game_message/table_option.hpp"
#include "../protocol/game_message/toggle_waitlist.hpp"
#include "../protocol/game_message/buyin_request.hpp"
#include "../automation/lobby.hpp"

namespace coinpoker {

void randomWait(int min = 1000, int max = 5000) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(min, max);
	int waitTime = dist(gen);
	std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
}

void delayAction(std::chrono::system_clock::time_point start, float min = 1.f, float max = 5.f) {
	std::random_device rd;
	std::mt19937 gen(rd());
	int minMilliseconds = min * 1000;
	int maxMilliseconds = max * 1000;
	std::uniform_int_distribution<int> dist(minMilliseconds, maxMilliseconds);
	auto waitTime = std::chrono::milliseconds(static_cast<uint32_t>(dist(gen)));
	std::this_thread::sleep_until(start + waitTime);
}

void Table::pushPacket(protocol::GamePacket&& packet) {
	{
		std::lock_guard<std::mutex> lock(mtx);
		packetQueue.push(std::move(packet));
	}
	cv.notify_one();
}

void Table::consume() {
	while (true) {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this] { return !packetQueue.empty() || isStopped.load(); });

		if (isStopped.load())
		{
			break;
		}

		protocol::GamePacket packet = std::move(packetQueue.front());
		packetQueue.pop();
		lock.unlock();
		handlePacket(packet);
	}
}

void Table::start() {
	consumerThread = std::thread{ [this] { consume(); } };
}

void Table::sendTableOptions() {
	protocol::TableOption r{
		.tableId = tableId,
		.autoPostAnte = true,
		.autoPostBlinds = true,
		.autoMuckWinning = true,
		.autoMuckLosing = true,
		.askForStraddle = false,
		.runBoardTwice = false,
		.waitForBB = true,
		.vrg = true,
		.autoTimeBank = true,
	};

	auto body = r.serialize();

	protocol::SendHeader header{
		.operationId = operationId,
		.tableId = tableId,
		.instanceId = instanceId,
		.serverTarget = serverTarget,
		.unused = 0,
		.unused1 = 0,
		.length = (uint16_t)(body.length() + 29)
	};
	protocol::GamePacketRequest gp(header, body);
	sendRequest(gp);
}

void Table::handlePacket(const protocol::GamePacket& packet) {
	protocol::MessageIds msgId = static_cast<protocol::MessageIds>(packet.getMessageId());

	switch (msgId) {
	case protocol::MessageIds::OnAction:
		onAction(protocol::Action::parse(packet.getBody()));
		break;
	case protocol::MessageIds::OnGameStart:
		onNewTableState(protocol::TableState::parse(packet.getBody()));
		break;
	case protocol::MessageIds::OnSeatUpdate:
		onSeatUpdate(protocol::SeatUpdate::parse(packet.getBody()));
		break;
	case protocol::MessageIds::OnEnterTable:
		onEnterTable(protocol::EnterTable::parse(packet.getBody()));
		break;
	case protocol::MessageIds::OnDealHoleCards:
		onDealingHoleCards(protocol::DealHoleCards::parse(packet.getBody()));
		break;
	case protocol::MessageIds::OnDealHeroCards:
		onDealingHeroCards(protocol::DealHeroCards::parse(packet.getBody()));
		break;
	case protocol::MessageIds::OnDealBoardCards:
		onBoardCards(protocol::BoardCards::parse(packet.getBody()));
		break;
	case protocol::MessageIds::OnNextTurn:
		onNextTurn(protocol::NextTurn::parse(packet.getBody()));
		break;
	case protocol::MessageIds::OnHeroTurn:
		onHeroTurn(protocol::HeroTurn::parse(packet.getBody()), packet.getTimestamp());
		break;
	case protocol::MessageIds::OnPotRakeUpdate:
		onPotRakeUpdate(protocol::PotRakeUpdate::parse(packet.getBody()));
		break;
	case protocol::MessageIds::OnPotUpdate:
		onPotUpdate(protocol::PotUpdate::parse(packet.getBody()));
		break;
	case protocol::MessageIds::OnGameEnd:
		onEndGame(protocol::EndGame::parse(packet.getBody()));
		break;
	case protocol::MessageIds::OnBuyIn:
		onBuyIn(protocol::BuyInPacket::parse(packet.getBody()));
		break;
	}
}

static auto PlayerEnumerator(TableState* state, auto&& callback) {
	using ResultType = std::invoke_result_t<decltype(callback), SeatState*>;
	std::vector<SeatState> players = state->getPlayers();
	return[index = 0u, callback, players]() mutable->std::optional<ResultType> {
		if (index < players.size()) {
			return callback(&players[index++]);
		}
		return std::nullopt;
		};
};

void Table::cancleBuyIn() {
	std::string body = std::format("buyincancel\t{}", tableId);

	protocol::SendHeader header{
		.operationId = operationId,
		.tableId = 0,
		.instanceId = instanceId,
		.serverTarget = serverTarget,
		.unused = 0,
		.unused1 = 0,
		.length = (uint16_t)(body.length() + 29)
	};

	protocol::GamePacketRequest gp(header, body);
	sendRequest(gp);
}

void Table::onBuyIn(const protocol::BuyInPacket& packet) {
	if (packet.type != "ring") return;

	randomWait(1000, 2000);
	if (!automation::Lobby::instance().shouldBuyIn(tableId)) {
		cancleBuyIn();
		randomWait(500, 1000);
		automation::Lobby::instance().closePokerWindow(tableId, automation::CloseReason::NORMAL);
		return;
	}

	auto user = automation::Lobby::instance().getUser();
	if (!user) {
		logger->error("User not set, aborting buyin");
		return;
	}

	protocol::BuyInRequest buyInRequest = {
		.cid = std::to_string(userId),
		.deposit_type_id = "0",
		.game_event_type_id = "0",
		.game_mod_id = "1",
		.game_mode_name = "NL Hold'em",
		.game_type_id = "1",
		.instance_id = std::to_string(instanceId),
		.name = tableName,
		.opid = std::to_string(packet.opId),
		.platform = "win32",
		.ssid = user->ssid,
		.table_id = std::to_string(tableId),
		.table_session_id = "",
		.totalAmount = std::to_string(packet.maxBuyIn)
	};

	std::string body = buyInRequest.serialize();

	protocol::SendHeader header{
		.operationId = 0,
		.tableId = 0,
		.instanceId = 0,
		.serverTarget = 0x6e,
		.unused = 0,
		.unused1 = 0,
		.length = (uint16_t)(body.length() + 29)
	};

	protocol::GamePacketRequest gp(header, body);
	sendRequest(gp);
	if (loneRing) {
		automation::Lobby::instance().changeTableState(tableId, automation::CurrentState::ALONE);
	}
	else {
		automation::Lobby::instance().changeTableState(tableId, automation::CurrentState::PLAYING);
	}
}

void Table::onDealingHoleCards(const protocol::DealHoleCards& packet) {
	if (!state || !state->isValid()) return;

	if (!state->isHeroPlaying()) {
		return;
	}

	if (state->sbIndex == -1) {
		logger->info("No small blind posted.");
	}

	auto hero = state->getHero();
	auto players = state->getPlayersAndDelar();
	if (players.size() < 3) {
		auto& firstPlayer = players.front();
		state->shouldFold = firstPlayer.index != state->dealerIndex;
	}
	else {
		auto& lastPlayer = players.back();
		state->shouldFold = lastPlayer.index != state->dealerIndex;
	}

	int bigBlindIndex = state->getBigBlindIndex();

	for (auto& seat : state->seats) {
		if (seat.second.index != bigBlindIndex && seat.second.postBlind) {
			seat.second.postAdditionalBlind = true;
		}
	}

	aiApiClient = std::make_unique<aiapi::Client>(utils::gConfig.aiApiAddress, utils::gConfig.aiApiPort);
	aiApiClient->StartGame("coin_poker",
		state->smallBlind,
		state->bigBlind,
		state->ante,
		state->straddle,
		0,
		hero->name,
		PlayerEnumerator(state.get(), [](SeatState* player)
			{
				return aiapi::PlayerInfo
				{
					.pid = player->name,
					.stack = player->initialStack,
				};
			}),
		[this]
		{
			std::vector<std::string> ids{};
			for (auto&& player : state->seats)
			{
				if (player.second.postAdditionalBlind) {
					ids.emplace_back(player.second.name);
				}
			}
			return ids;
		}());
	suggestion = aiApiClient->PrivateHand(hero->holeCards);
}

void Table::onDealingHeroCards(const protocol::DealHeroCards& packet) {
	if (!state || !state->isValid()) return;
	auto it = state->seats.find(packet.seatIndex);
	if (it != state->seats.end()) {
		logger->info("Hero cards delt {}", packet.cards);
		it->second.holeCards = utils::splitCards(packet.cards);
	}
}

void Table::onAction(const protocol::Action& packet) {
	if (!state || !state->isValid()) return;
	auto it = state->seats.find(packet.seatIndex);
	if (it == state->seats.end()) return;
	auto& player = it->second;

	aiapi::ActionType type = aiapi::ActionType_Invalid;
	player.stack = packet.stack;

	uint64_t amount = 0;

	//TODO: parse action as enum
	if (packet.action == "S") {
		state->sbIndex = packet.seatIndex;
		player.initialStatus = "T";
	}
	else if (packet.action == "N") {
		player.initialStatus = "W";
	}
	else if (packet.action == "Z" || packet.action == "ZR") {
		state->straddle = packet.amount;
	}
	else if (packet.action == "G") {
		player.postBlind = true;
		//sometimes it happens that the player is sit out, but still posts a blind
		//in these cases we want to set the initalStatus to playing. 
		player.initialStatus = "T";
	}
	else if (packet.action == "H") {
		player.holeCards = utils::splitCards(packet.showCards);
	}
	else if ((packet.action == "B" || packet.action == "R" || packet.action == "L") && packet.stack == 0) {
		type = aiapi::ActionType_Allin;
		amount = packet.amount;
		logger->info("Player {} allin {}.", player.name, amount);
	}
	else if (packet.action == "B") {
		type = aiapi::ActionType_Bet;
		amount = packet.amount;
		logger->info("Player {} bet {}.", player.name, amount);
	}
	else if (packet.action == "R") {
		type = aiapi::ActionType_Raise;
		amount = packet.commitedAmount - packet.amountToCall;
		if (state->stage == PREFLOP)
			state->preflopRaiseCount++;
		logger->info("Player {} raised {}.", player.name, amount);
	}
	else if (packet.action == "L") {
		type = aiapi::ActionType_Call;
		amount = packet.amount;
		logger->info("Player {} called {}.", player.name, amount);
	}
	else if (packet.action == "C") {
		type = aiapi::ActionType_Check;
		amount = 0;
		logger->info("Player {} checked.", player.name);
	}
	else if (packet.action == "F") {
		type = aiapi::ActionType_Fold;
		logger->info("Player {} folded.", player.name);
		amount = 0;
	}

	if (aiApiClient && type != aiapi::ActionType_Invalid) {
		suggestion = aiApiClient->PlayerMove(player.name, type, amount);
	}
}

void Table::takeSeat(int seatIndex) {
	protocol::TakeSeat seat;
	seat.tableId = tableId;
	seat.userId = userId;
	seat.tableSeat = seatIndex;

	std::string body = seat.serialize();

	protocol::SendHeader header{
		.operationId = operationId,
		.tableId = 0,
		.instanceId = instanceId,
		.serverTarget = serverTarget,
		.unused = 0,
		.unused1 = 0,
		.length = (uint16_t)(body.length() + 29)
	};

	protocol::GamePacketRequest gp(header, body);
	sendRequest(gp);
	//sendTableOptions();
}

void Table::takeSeatOrWait(const std::vector<protocol::Seat>& seats) {
	int availableSeat = -1;
	randomWait(500, 1000);
	for (auto& seat : seats) {
		if (seat.status == "O") {
			availableSeat = seat.index;
			break;
		}
	}
	if (availableSeat != -1) {
		takeSeat(availableSeat);
	}
	else {
		toggleWaitList(true);
		randomWait(1000, 2000);
		automation::Lobby::instance().closePokerWindow(tableId, automation::CloseReason::WAITING);
	}
}

void Table::toggleWaitList(bool value) {
	protocol::ToggleWaitList packet;
	packet.tableId = tableId;
	packet.userId = tableId;
	packet.value = !value;

	std::string body = packet.serialize();

	protocol::SendHeader header{
		.operationId = operationId,
		.tableId = tableId,
		.instanceId = instanceId,
		.serverTarget = serverTarget,
		.unused = 0,
		.unused1 = 0,
		.length = (uint16_t)(body.length() + 29)
	};

	protocol::GamePacketRequest gp(header, body);
	sendRequest(gp);
}

/*
void Table::checkLoneRing(const protocol::TableState& packet) {
	if (packet.tableSize == 2) return;
	if (packet.state != "idle") return;
	int activePlayers = 0;
	for (const auto& seat : packet.seats) {
		if (seat.status != "O")  activePlayers++;
	}
	if (activePlayers > 1) return;
	if (activePlayers == 1 && heroIndex == -1) return;
	loneRing = true;
	automation::Lobby::instance().changeTableState(tableId, automation::CurrentState::ALONE);
}*/

void Table::checkLoneRing() {
	if (!state) return;
	if (state->tableSize == 2) return;
	int activePlayers = 0;
	for (const auto& seat : state->seats) {
		if (seat.second.status != "O" && seat.second.status != "X") {
			activePlayers++;
		}
	}
	if (activePlayers > 1) return;
	if (activePlayers == 1 && heroIndex == -1) return;
	loneRing = true;
	automation::Lobby::instance().changeTableState(tableId, automation::CurrentState::ALONE);
}


void Table::onNewTableState(const protocol::TableState& packet) {
	if (heroIndex == -1) {
		auto user = automation::Lobby::instance().getUser();
		if (!user) {
			logger->error("Current user not set.");
			return;
		}

		for (const auto& seat : packet.seats) {
			if (seat.name == user->username) {
				heroIndex = seat.index;
				break;
			}
		}
	}
	if (heroIndex == -1) {
		takeSeatOrWait(packet.seats);
	}
	tableName = packet.name;
	state = std::make_unique<TableState>();
	logger->info("New round starting...");
	state->dealerIndex = packet.dealerPos;
	state->activePlayers = packet.activePlayers;
	state->tableSize = packet.tableSize;
	state->heroIndex = heroIndex;
	state->ante = packet.ante;
	state->stage = PREFLOP;
	state->bigBlind = packet.bigBlind;
	state->smallBlind = packet.smallBlind;
	state->setSeats(packet.seats);
	state->mode = packet.state;
	checkLoneRing();
	checkHeroStack();

	if (!state->isHeroPlaying()) {
		aiApiClient.reset(nullptr);
		auto hero = state->getHero();
		if (hero && hero->status == "Z" && utils::gConfig.autoSitIn && !sitout) {
			setSitOutNextRound(false);
		}
		else if (hero && hero->status == "Z" && sitout) {
			automation::Lobby::instance().closePokerWindow(tableId, sitOutReason);
		}
	}

	if (loneRing && state->isValid()) {
		loneRing = false;
		automation::Lobby::instance().changeTableState(tableId, automation::CurrentState::PLAYING);
	}
}

//TODO make this return boolean instead
void Table::checkHeroStack() {
	auto hero = state->getHero();
	if (!hero.has_value()) return;
	auto heroStackBB = static_cast<double>(hero->initialStack) / state->bigBlind;
	if (heroStackBB > utils::gConfig.maxStackBB) {
		sitOut(automation::CloseReason::COOLDOWN);
	}
}

void Table::sitOut(automation::CloseReason reason) {
	sitOutReason = reason;
	sitout = true;
	setSitOutNextRound(true);
}

void Table::onSeatUpdate(const protocol::SeatUpdate& packet) {
	logger->info("Seat update called: index: {} stack: {}, status: {}", packet.index, packet.stack, packet.status);
	if (state) {
		state->updateSeat(packet);
	}
	if (packet.index == heroIndex && packet.status == "O") {
		logger->debug("Hero left the table.");
		heroIndex = -1;
		automation::Lobby::instance().closePokerWindow(tableId, sitOutReason);
	}
	else if (packet.index != heroIndex && packet.status == "O") {
		checkLoneRing();
	}
	else if (packet.index == heroIndex && packet.status == "Z") {
		logger->debug("Hero sit out.");
		if (utils::gConfig.autoSitIn && !sitout) {
			randomWait(2000, 3000);
			setSitOutNextRound(false);
		}
		else if (sitout) {
			randomWait(1000, 2000);
			automation::Lobby::instance().closePokerWindow(tableId, sitOutReason);
		}
	}
}

void Table::onPotUpdate(const protocol::PotUpdate& packet) {
	if (!state || !state->isValid()) return;
	for (const auto& pot : packet.pots) {
		if (pot.fromType == "S") {
			auto& p = state->seatPots[pot.fromPosition];
			p.index = pot.fromPosition;
			p.amount = pot.totalAmountFrom;
		}
		else if (pot.fromType == "P") {
			auto& p = state->pots[pot.fromPosition];
			p.index = pot.fromPosition;
			p.amount = pot.totalAmountFrom;
		}
		if (pot.toType == "P") {
			auto& p = state->pots[pot.toPosition];
			p.index = pot.toPosition;
			p.amount = pot.totalAmountTo;
		}
		else if (pot.toType == "S") {
			auto& p = state->seatPots[pot.toPosition];
			auto& fromPot = pot.fromType == "P" ? state->pots[pot.fromPosition] : state->seatPots[pot.fromPosition];
			p.index = pot.toPosition;
			p.amount = pot.totalAmountTo;
			fromPot.winners.push_back(pot.toPosition);
		}
	}
}

void Table::onPotRakeUpdate(const protocol::PotRakeUpdate& packet) {
	if (!state || !state->isValid()) return;
	logger->info("pot rake: {}", packet.toJson().dump());
	for (const auto& pot : packet.pots) {
		auto it = state->pots.find(pot.index);
		if (it == state->pots.end()) continue;
		it->second.amount = pot.finalAmount;
		it->second.rakeAmount = pot.rake;
	}
}

void Table::onEnterTable(const protocol::EnterTable& packet) {
	heroIndex = packet.seatIndex;
	if (!state || !state->isValid()) return;
	state->heroIndex = packet.seatIndex;
}

void Table::onNextTurn(const protocol::NextTurn& packet) {
	if (!state || !state->isValid()) return;
}

void Table::sendAction(const aiapi::ActionSuggestion& action, const protocol::HeroTurn& packet) {
	std::string actionString = "";
	std::string amountString = "";
	if (action.type == aiapi::ActionType_Check) {
		actionString = "call";
		amountString = "0";
	}
	else if (action.type == aiapi::ActionType_Call) {
		actionString = "call";
		amountString = std::to_string(packet.amountToCall);
	}
	else if (action.type == aiapi::ActionType_Bet) {
		actionString = "bet";
		amountString = std::to_string(action.amount);
	}
	else if (action.type == aiapi::ActionType_Raise) {
		actionString = "bet";
		amountString = std::to_string(suggestion->amount + packet.amountToCall + packet.commitedAmount);
	}
	else if (action.type == aiapi::ActionType_Fold) {
		actionString = "fold";
		amountString = "0";
	}
	else if (action.type == aiapi::ActionType_Allin) {
		if (packet.amountToCall == packet.stack) {
			actionString = "call";
			amountString = std::to_string(packet.amountToCall);
		}
		else {
			actionString = "bet";
			amountString = std::to_string(packet.maxRaise);
		}
	}

	std::string body = actionString + "\t" + std::to_string(tableId) + "\t" + amountString;

	protocol::SendHeader header{
		.operationId = operationId,
		.tableId = tableId,
		.instanceId = instanceId,
		.serverTarget = serverTarget,
		.unused = 0,
		.unused1 = 0,
		.length = (uint16_t)(body.length() + 29)
	};
	protocol::GamePacketRequest gp(header, body);
	sendRequest(gp);
}

void Table::onHeroTurn(const protocol::HeroTurn& packet, std::chrono::system_clock::time_point timestamp) {
	logger->info("Hero's turn to play");
	if (!state || !state->isValid()) {
		const bool canCheck = packet.amountToCall == 0;
		auto actionType = canCheck ? aiapi::ActionType_Check : aiapi::ActionType_Fold;
		sendAction(aiapi::ActionSuggestion{ .type = actionType, .amount = 0 }, packet);
	}
	utils::ActionDelay delay = { .min = 2.5f, .max = 3.5f };
	switch (state->stage) {
	case PREFLOP: {
		if (state->preflopRaiseCount < 1) 
			delay = utils::gConfig.preflopActionDelay; 
		else
			delay = utils::gConfig.preflopActionRaiseDelay; 
		break;
	}
	case FLOP: delay = utils::gConfig.flopActionDelay; break;
	case TURN: delay = utils::gConfig.turnActionDelay; break;
	case RIVER: delay = utils::gConfig.riverActionDelay; break;
	}

	delayAction(timestamp, delay.min, delay.max);

	if (state->shouldFold) {
		logger->info("Folding becuase of the wrong order.");
		sendAction(aiapi::ActionSuggestion{ .type = aiapi::ActionType_Fold, .amount = 0 }, packet);
	}
	else if (suggestion.has_value()) {
		sendAction(suggestion.value(), packet);
		suggestion.reset();
	}
	else if (packet.amountToCall == 0) {
		sendAction(aiapi::ActionSuggestion{ .type = aiapi::ActionType_Check, .amount = 0 }, packet);
	}
	else {
		sendAction(aiapi::ActionSuggestion{ .type = aiapi::ActionType_Fold, .amount = 0 }, packet);
	}
}

void Table::onEndGame(const protocol::EndGame& packet) {
	if (!state || !state->isValid()) return;

	logger->info("Round ended.");

	for (const auto& [index, seatPot] : state->seatPots) {
		state->seats[index].stack += seatPot.amount;
	}

	for (const auto& [index, pot] : state->pots) {
		for (const auto& winner : pot.winners) {
			state->seats[winner].rakeAmount = pot.rakeAmount / pot.winners.size();
		}
	}

	for (const auto& [index, seat] : state->seats) {
		logger->info("Seat pot amount: {} {} {} {} {}",
			state->seats[index].name,
			state->seats[index].stack,
			state->seats[index].initialStack,
			static_cast<int64_t>(state->seats[index].stack) - static_cast<int64_t>(state->seats[index].initialStack),
			seat.rakeAmount);
	}

	if (aiApiClient) {
		aiApiClient->Showdown(
			PlayerEnumerator(state.get(), [](SeatState* player)
				{
					const int64_t payoff = static_cast<int64_t>(player->stack) - static_cast<int64_t>(player->initialStack);
					auto holeCards = !player->holeCards.empty() ? player->holeCards : std::vector<std::string>{};
					return aiapi::ShowdownInfo
					{
						.pid = player->name,
						.payoff = payoff,
						.rake = -static_cast<int64_t>(player->rakeAmount),
						.showdown_cards = holeCards
					};
				})
		);
		aiApiClient.reset(nullptr);
	}
	else {
		logger->info("Done: {}", packet.toJson().dump());
	}
}

void Table::onBoardCards(const protocol::BoardCards& packet) {
	if (!state || !state->isValid()) return;

	switch (state->stage) {
	case PREFLOP: state->stage = FLOP; break;
	case FLOP:    state->stage = TURN; break;
	case TURN:    state->stage = RIVER; break;
	default: break;
	}

	if (aiApiClient) {
		logger->info("Board cards dealt {}", packet.cards);
		suggestion = aiApiClient->DealCommunityCards(utils::splitCards(packet.cards));
	}
}

void Table::setWaitForBB(bool waitBB) {
	std::string value = waitBB ? "1" : "0";
	setOption("WBB", value);
}

void Table::setSitOutNextRound(bool sitout) {
	std::string value = sitout ? "1" : "0";
	setOption("SON", value);
}

void Table::setOption(std::string option, std::string value) {
	std::string body = "option\t" + std::to_string(tableId) + "\t" + option + "\t" + value;
	protocol::SendHeader header{
		.operationId = operationId,
		.tableId = tableId,
		.instanceId = instanceId,
		.serverTarget = serverTarget,
		.unused = 0,
		.unused1 = 0,
		.length = (uint16_t)(body.length() + 29)
	};
	protocol::GamePacketRequest gp(header, body);
	sendRequest(gp);
}

std::optional<protocol::GamePacketRequest> Table::onSendPacket(protocol::GamePacketRequest& packet) {
	if (packet.getType() == "option") {
		return modifyOptions(packet);
	}
	return std::nullopt;
}

protocol::GamePacketRequest Table::modifyOptions(protocol::GamePacketRequest& packet) {
	auto parts = packet.getBodyParts();
	for (int i = 2; i < parts.size(); i += 2) {
		if (parts[i] == "WBB") {
			auto value = utils::gConfig.waitForBB ? "1" : "0";
			parts[i + 1] = value;
		}
	}
	auto newBody = utils::joinStrings(parts, "\t");
	protocol::GamePacketRequest gp(packet.getHeader(), newBody);
	return gp;
}

uint64_t Table::getHeroStack() {
	std::lock_guard<std::mutex> lock(mtx);
	if (!state) return 0;
	auto hero = state->getHero();
	if (!hero) return 0;
	return hero->stack;
}

void Table::setWebSocket(std::shared_ptr<Websocket> socket) {
	ws.store(std::move(socket), std::memory_order_relaxed);
}

void Table::sendRequest(protocol::GamePacketRequest& gp) {
	auto currentWs = ws.load(std::memory_order_relaxed);
	Websocket::sendMessage(currentWs, gp);
}

}

