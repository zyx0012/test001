#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include "table_state.hpp"
#include "../protocol/game_packet.hpp"
#include "../protocol/game_message/action.hpp"
#include "../protocol/game_message/start_game.hpp"
#include "../protocol/game_message/pot_update.hpp"
#include "../protocol/game_message/seat_update.hpp"
#include "../protocol/game_message/next_turn.hpp"
#include "../protocol/game_message/end_game.hpp"
#include "../protocol/game_message/board_cards.hpp"
#include "../protocol/game_message/enter_table.hpp"
#include "../protocol/game_message/deal_hold_cards.hpp"
#include "../protocol/game_message/deal_hero_cards.hpp"
#include "../protocol/game_message/pot_rake_update.hpp"
#include "../protocol/game_message/buy_in.hpp"
#include "../AIApi/Client.hpp"
#include "../AIApi/Protocol.hpp"
#include "../hooks/websocket.hpp"
#include "../protocol/game_message/hero_turn.hpp"
#include "../utils/logging.hpp"
#include "../automation/lobby_types.hpp"
#include <chrono>

namespace coinpoker {


class Table {
public:
	Table(int tableId, int instanceId, int userId, int operationId, int serverTarget, std::shared_ptr<Websocket> ws) :
		logger(utils::CreateLoggerNoPrefix(std::format("table[{}]", tableId))),
		tableId(tableId),
		instanceId(instanceId),
		userId(userId),
		operationId(operationId),
		serverTarget(serverTarget),
		ws(ws) {}

	void pushPacket(protocol::GamePacket&&);
	std::optional<protocol::GamePacketRequest> onSendPacket(protocol::GamePacketRequest& packet);
	void start();
	void sitOut(automation::CloseReason reason =  automation::CloseReason::NORMAL);
	uint64_t getHeroStack();
	void setWebSocket(std::shared_ptr<Websocket> socket);
	void sendRequest(protocol::GamePacketRequest& gp);
	inline uint32_t getId() { return tableId; };

	automation::CloseReason sitOutReason = automation::CloseReason::NORMAL;
	std::atomic_bool isStopped{ false };
	std::mutex mtx{};
	std::condition_variable cv{};
	std::thread consumerThread;

private:
	utils::Logger logger;
	uint32_t tableId;
	uint32_t instanceId;
	uint32_t userId;
	uint32_t operationId;
	uint32_t serverTarget;
	std::string tableName{};
	int heroIndex = -1;
	std::atomic_bool sitout;
	bool loneRing = false;
	std::optional<aiapi::ActionSuggestion> suggestion{};
	std::queue<protocol::GamePacket> packetQueue{};
	std::unique_ptr<aiapi::Client> aiApiClient{};
	std::unique_ptr<TableState> state{};

	std::atomic<std::shared_ptr<Websocket>> ws{};

	void consume();
	void handlePacket(const protocol::GamePacket& packet);

	void onAction(const protocol::Action& packet);
	void cancleBuyIn();
	void onNewTableState(const protocol::TableState& packet);
	void onSeatUpdate(const protocol::SeatUpdate& packet);
	void onPotUpdate(const protocol::PotUpdate& packet);
	void onPotRakeUpdate(const protocol::PotRakeUpdate& packet);
	void onNextTurn(const protocol::NextTurn& packet);
	void onHeroTurn(const protocol::HeroTurn& packet, std::chrono::system_clock::time_point timestamp);
	void onEndGame(const protocol::EndGame& packet);
	void onBoardCards(const protocol::BoardCards& packet);
	void onEnterTable(const protocol::EnterTable& packet);
	void onDealingHoleCards(const protocol::DealHoleCards& packet);
	void onDealingHeroCards(const protocol::DealHeroCards& packet);
	void sendAction(const aiapi::ActionSuggestion& action, const protocol::HeroTurn& packet);
	void setSitOutNextRound(bool sitout);
	void setWaitForBB(bool value);
	void setOption(std::string option, std::string value);
	void takeSeatOrWait(const std::vector<protocol::Seat>& seats);
	void takeSeat(int seatIndex);
	void sendTableOptions();
	void onBuyIn(const protocol::BuyInPacket& packet);
	void toggleWaitList(bool value);
	void checkHeroStack();
	//void checkLoneRing(const protocol::TableState& packet);
	void checkLoneRing();
	protocol::GamePacketRequest modifyOptions(protocol::GamePacketRequest& packet);
};
}
