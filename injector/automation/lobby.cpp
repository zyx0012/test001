#include "./lobby.hpp"
#include "QtWidgets/qshortcut.h"
#include <QtGui/QKeyEvent>
#include "../protocol/game_message/budget_update.hpp"
#include <thread>
#include <chrono>
#include <random>


namespace coinpoker {
namespace automation {

class LobbyKeyLogger : public QObject {
public:
	using QObject::QObject;
	bool eventFilter(QObject* obj, QEvent* event) override {
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			Qt::KeyboardModifiers mods = keyEvent->modifiers();
			if ((mods & Qt::ControlModifier) && (mods & Qt::AltModifier) && keyEvent->key() == Qt::Key_E) {
				QWidget* w = qobject_cast<QWidget*>(obj);
				if (w) {
					w->window()->setStyleSheet("background-color: purple;");
				}
				Lobby::instance().endSession();
			}
		}
		return false;
	}
};

bool LobbyListHandler::matches(QWidget* w, QEvent* event) const {
	return event->type() == QEvent::ShowToParent &&
		w->objectName() == "ListBox" &&
		QString(w->metaObject()->className()) == "CustomTableView";
}

void LobbyListHandler::handle(QWidget* w, QEvent* event) {
	auto* table = qobject_cast<QTableView*>(w);
	if (!table) return;
	Lobby::instance().setLobbyWidget(table);
	if (!keyLoggerInstalled) {
		table->window()->installEventFilter(new LobbyKeyLogger());
		keyLoggerInstalled = true;
	}
};

Lobby& Lobby::instance() {
	static Lobby instance;
	return instance;
}

Lobby::Lobby() {
	createdAt_ = clock::now();
	logger = utils::CreateLoggerNoPrefix("lobby");

	const auto& cfg = utils::gConfig;
	breaks_ = cfg.breaks;

	int breakMaxOffset = cfg.breaksDuration * 0.1;
	int sessionMaxOffset = cfg.sessionLength * 0.1;
	
	std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> distBreaks(-breakMaxOffset, breakMaxOffset);
	std::uniform_int_distribution<int> distSession(-sessionMaxOffset, sessionMaxOffset);
	sessionLength_ = cfg.sessionLength + distSession(rng);

	for (int i = 0; i < breaks_; ++i) {
		breaksDuration_.emplace_back(std::chrono::minutes(cfg.breaksDuration + distBreaks(rng)));
	}
};

Lobby::~Lobby() {
	isStopped = true;
	if (workerThread_.joinable()) {
		workerThread_.join();
	}
}

void Lobby::closePokerWindow(uint32_t tableId, CloseReason reason) {
	std::unique_lock lock{ mutex_ };
	auto t = currentTables.find(tableId);
	if (t == currentTables.end()) return;
	logger->info("Table {} is closing.", tableId);
	if (reason == CloseReason::COOLDOWN) {
		cooldownTables[tableId] = clock::now();
	}
	if (reason == CloseReason::WAITING) {
		t->second->state = CurrentState::WAITING;
	}
	else {
		t->second->state = CurrentState::CLOSED;
	}

	if (t->second->state == CurrentState::WAITING) {
		//If waiting we close imediatly
		auto widget = t->second->tableWidget.getWidget();
		QMetaObject::invokeMethod(
			widget,
			[widget]() { widget->close(); },
			Qt::QueuedConnection
		);
	}
}

void Lobby::addPokerTable(QWidget* widget) {
	PokerTableWrapper b(widget);
	{
		std::unique_lock lock{ mutex_ };
		auto id = b.getTableId();
		logger->info("Table {} is opening.", id);
		auto t = currentTables.find(id);
		if (t != currentTables.end()) {
			t->second->tableWidget = b;
			if (t->second->state != CurrentState::WAITING) {
				t->second->state = CurrentState::OPENED;
			}
		}
		else {
			logger->info("Closing table {} because it was not opened by automation.", id);
			QMetaObject::invokeMethod(
				widget,
				[widget]() { widget->close(); },
				Qt::QueuedConnection
			);
		}
	}
}

void Lobby::removePokerTable(QWidget* widget) {
	PokerTableWrapper b(widget);
	{
		std::unique_lock lock{ mutex_ };
		auto tableId = b.getTableId();
		auto t = currentTables.find(tableId);
		logger->info("Closing table {}", tableId);
		if (t == currentTables.end()) return;
		if (t->second->state != CurrentState::WAITING) {
			currentTables.erase(tableId);
		}
	}
}

bool Lobby::isBreakTime(std::chrono::minutes time) {
	if (breaks_ <= 0) {
		return false;
	}
	auto space = std::chrono::minutes(sessionLength_) / (breaks_ + 1);
	for (int i = 1; i <= breaks_; i++) {
		auto center = space * i;
        auto start = center - breaksDuration_[i - 1] / 2;
        auto end = start + breaksDuration_[i - 1];

		if (time >= start && time < end) {
			return true;
		}
	}
	return false;
}

void Lobby::onLobbyPacket(std::shared_ptr<Websocket> ws_, int msgId, protocol::StreamReader& reader) {
	if (msgId == 0x1f0000) {
		{
			std::lock_guard<std::mutex> lock(mutex_);
			packetQueue_.push(std::move(reader));
		}
		this->ws = ws_;
		cond_.notify_one();
	}
	else if (msgId == 0x80000) {
		auto packet = protocol::LoginReponse::parse(reader);
		user_ = UserData{
			.userId = packet.userId,
			.balance = packet.usdtBalance,
			.username = packet.userName,
			.email = packet.email,
			.ssid = packet.ssid,
		};
	}
}

void Lobby::doClosePokerWindow() {
	auto now = clock::now();
	if (now - lastTimestamp < std::chrono::seconds(1)) return;
	for (const auto& [tableId, table] : currentTables) {
		if (table->state == CurrentState::CLOSED) {
			auto widget = table->tableWidget.getWidget();
			QMetaObject::invokeMethod(
				widget,
				[widget]() { widget->close(); },
				Qt::QueuedConnection
			);
			lastTimestamp = now;
			return;
		}
	}
}

void Lobby::doSitOut() {
	auto now = clock::now();
	if (now - lastTimestamp < std::chrono::seconds(1)) return;
	for (const auto& [tableId, table] : currentTables) {
		if (table->state == CurrentState::ALONE || table->state == CurrentState::PLAYING) {
			table->state = CurrentState::SITTING_OUT;
			TableController::SitOut(table->id);
			lastTimestamp = now;
			return;
		}
	}
}


void Lobby::run() {
	std::unique_lock lock(mutex_);
	auto now = clock::now();
	doClosePokerWindow();
	if (isSessionStopped) {
		doSitOut();
		int countOpenTables = 0;
		for (const auto& [tableId, table] : currentTables) {
			if (table->state == CurrentState::PLAYING || table->state == CurrentState::ALONE || table->state == CurrentState::SITTING_OUT) {
				countOpenTables++;
			}
		}
		if (countOpenTables == 0 && now - lastTimestamp > std::chrono::seconds(10)) {
			isStopped = true;
			HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "Local\\CoinPokerWatchdogShutdown");
			if (hEvent) {
				SetEvent(hEvent);
				CloseHandle(hEvent);
			}
			QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
		}
		return;
	}

	if (isBreakTime(std::chrono::duration_cast<std::chrono::minutes>(now - sessionStartedAt_))) {
		if (isBreak == false) {
			logger->info("Taking a break...");
			//TableController::SitOutAll();
			isBreak = true;
		}
		else {
			doSitOut();
		}
		return;
	}
	else {
		isBreak = false;
	}

	if (now - sessionStartedAt_ > std::chrono::minutes(sessionLength_)) {
		logger->info("Session has ended. Wating until all tables are closed before closing the the main window...");
		//TableController::SitOutAll();
		isSessionStopped = true;
		return;
	}
	if (now - createdAt_ < std::chrono::seconds(10)) return;
	auto& tableConfigs = utils::gConfig.tables;
	for (int i = 0; i < tableConfigs.size(); i++) {
		if (now - lastTimestamp < std::chrono::seconds(2)) return;
		auto& config = tableConfigs[i];
		configRun(config, i);
	}
}

void Lobby::updateLobbyItems(protocol::StreamReader& reader) {
	auto packet = protocol::LobbyData::parse(reader);
	{
		std::unique_lock lock{ mutex_ };
		for (auto& item : packet.items) {
			lobbyItems[item.tableId] = std::move(item);
		}
	}
	run();
}

void Lobby::consumerThread() {
	while (true) {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] { return !packetQueue_.empty() || isStopped.load(); });

		if (isStopped.load()) break;

		protocol::StreamReader packet = std::move(packetQueue_.front());
		packetQueue_.pop();
		lock.unlock();
		updateLobbyItems(packet);
	}
}

void Lobby::reportBalance() {
	while (!isSessionStopped) {
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (user_) {
				auto playingBalance = TableController::getPlayingBalance();
				auto totalBalance = user_->balance + playingBalance;
				logger->info("Balance report, username: {}, balance: {}", user_->username, totalBalance);
			}
		}
		std::this_thread::sleep_for(std::chrono::minutes(1));
	}
}

void Lobby::start() {
	auto config = readSharedConfig();
	if (config) {
		sessionStartedAt_ = clock::time_point(clock::duration(config->sessionStartTime));
	}
	else {
		sessionStartedAt_ = createdAt_;
	}
	workerThread_ = std::thread(&Lobby::consumerThread, this);
	std::thread(&Lobby::reportBalance, this).detach();
	logger->info("Lobby automation started. createdAt: {}, sessionStartedAt: {}.",
		std::format("{:%Y-%m-%d %H:%M:%S}", createdAt_),
		std::format("{:%Y-%m-%d %H:%M:%S}", sessionStartedAt_)
	);
}

void Lobby::configRun(utils::TableConfig& config, int configIndex) {
	std::unordered_set<uint32_t> currentMatched;
	std::unordered_set<uint32_t> waitingMatched;
	bool hasEmptyTable = false;

	for (const auto& [tableId, table] : currentTables) {
		if (table->configIndex != configIndex) continue;
		if (table->state == CurrentState::WAITING) {
			waitingMatched.insert(tableId);
		}
		else if (table->state == CurrentState::ALONE) {
			hasEmptyTable = true;
			currentMatched.insert(tableId);
		}
		else if (table->state != CurrentState::CLOSED) {
			currentMatched.insert(tableId);
		}
	}

	if (currentMatched.size() >= config.tableCount) return;

	bool includeEmptyTables = (config.emptyTable && !hasEmptyTable) || config.tableSize == 2;
	std::vector<protocol::LobbyItem> matches;
	for (auto& [tableId, item] : lobbyItems) {
		if (
			item.ops == 0 && match(item, config) &&
			!currentMatched.contains(tableId) &&
			(!cooldownTables.contains(tableId) || clock::now() - cooldownTables[tableId] > std::chrono::minutes(utils::gConfig.tableCooldown)) &&
			!waitingMatched.contains(tableId) &&
			//(item.activePlayers == 1 || item.activePlayers == 0) &&
			(item.activePlayers > 0 || includeEmptyTables))
		{
			matches.push_back(item);
		}
	}

	std::sort(matches.begin(), matches.end(), [](const auto& a, const auto& b) {
		int availA = static_cast<int>(a.maxPlayers) - static_cast<int>(a.activePlayers);
		int availB = static_cast<int>(b.maxPlayers) - static_cast<int>(b.activePlayers);

		if (availA == 0 && availB != 0) return false;
		if (availB == 0 && availA != 0) return true;

		if (a.activePlayers == 0 && b.activePlayers != 0) return false;
		if (b.activePlayers == 0 && a.activePlayers != 0) return true;

		if (availA == 0 && availB == 0) return a.waitingPlayers <= b.waitingPlayers;
		return availA <= availB;
		});

	if (matches.empty()) return;
	auto& table = matches[0];
	auto t = std::make_shared<CurrentTable>(CurrentTable{
		.id = table.tableId,
		.configIndex = configIndex,
		.gameType = table.gameType,
		.tableType = table.tableType,
		.blinds = table.blinds,
		.maxPlayers = table.maxPlayers,
		.state = CurrentState::OPENING,
		});
	QMetaObject::invokeMethod(
		lobbyTable_,
		[this, t]() { openTable(t); },
		Qt::QueuedConnection
	);
	lastTimestamp = clock::now();
}

bool Lobby::match(const protocol::LobbyItem& item, utils::TableConfig& config) {
	int sb = static_cast<int>(std::floor(config.stakes.sb * 100));
	int bb = static_cast<int>(std::floor(config.stakes.bb * 100));
	bool configHasAnte = config.stakes.ante > 0;
	bool itemHasAnte = false;
	bool isPrivate = false;
	bool isSpecial = false;
	for (auto type : item.tableType) {
		if (type == TableType::Ante) {
			itemHasAnte = true;
		}
		else if (type == TableType::Private) {
			isPrivate = true;
		}
		else if (type == TableType::Special || type == TableType::SpecialHS) {
			isSpecial = true;
		}
	}
	return !isPrivate && !isSpecial && item.maxPlayers == config.tableSize &&
		item.blinds.first == sb &&
		item.blinds.second == bb &&
		item.gameType == 1 &&
		itemHasAnte == configHasAnte;
}

void Lobby::setLobbyWidget(QTableView* w) {
	std::unique_lock lock(mutex_);
	lobbyTable_ = w;
}

bool Lobby::shouldBuyIn(uint32_t tableId) {
	std::unique_lock lock(mutex_);
	if (isSessionStopped) return false;
	if (isBreak) return false;
	auto t = currentTables.find(tableId);
	if (t == currentTables.end()) return false;
	auto table = t->second;
	if (table->state != CurrentState::WAITING) return true;
	auto& tableConfigs = utils::gConfig.tables;
	int configIndex = table->configIndex;
	auto& config = tableConfigs[configIndex];
	int tables = 0;
	for (const auto& [tableId, t] : currentTables) {
		if (t->configIndex == configIndex &&
			(t->state == CurrentState::PLAYING || t->state == CurrentState::ALONE || t->state == CurrentState::SITTING_OUT)
			) {
			tables++;
		}
	}
	return tables < config.tableCount;
}

void Lobby::changeTableState(uint32_t tableId, CurrentState state) {
	std::unique_lock lock(mutex_);
	auto t = currentTables.find(tableId);
	if (t == currentTables.end()) return;
	auto table = t->second;
	table->state = state;
	if (state == CurrentState::ALONE) {
		checkLoneTables(table->configIndex);
	}
}

void Lobby::checkLoneTables(int configIndex) {
	std::vector<uint32_t> loneTables;
	for (const auto& [tableId, table] : currentTables) {
		if (table->configIndex == configIndex && table->state == CurrentState::ALONE) {
			loneTables.push_back(tableId);
		}
	}
	if (loneTables.size() <= 1) return;
	for (int i = 0; i < loneTables.size() - 1; i++) {
		auto tableId = loneTables[i];
		currentTables[tableId]->state = CurrentState::CLOSED;
		auto widget = currentTables[tableId]->tableWidget.getWidget();
		QMetaObject::invokeMethod(
			widget,
			[widget]() { widget->close(); },
			Qt::QueuedConnection
		);
	}
}

void Lobby::openTable(std::shared_ptr<CurrentTable> table) {
	if (!lobbyTable_) return;
	auto model = lobbyTable_->model();
	if (!model) return;
	int rowCount = model->rowCount();
	for (int i = 0; i < rowCount; i++) {
		auto index = model->index(i, 0);
		uint32_t id = model->data(index, 266).toUInt();
		if (table->id == id)
		{
			{
				std::unique_lock lock(mutex_);
				currentTables[table->id] = table;
			}
			lobbyTable_->setCurrentIndex(index);
			lobbyTable_->activated(index);
			lobbyTable_->scrollTo(index);
			lobbyTable_->doubleClicked(index);
			break;
		}
	}
}

void Lobby::endSession() {
	logger->info("Session has ended. Wating until all tables are closed before closing the the main window...");
	isSessionStopped = true;
}

std::optional<UserData> Lobby::getUser() {
	std::unique_lock lock(mutex_);
	return user_;
}

void Lobby::onBudgetChange(protocol::GamePacket& packet) {
	auto data = protocol::BudgetUpdate::parse(packet.getBody());
	std::unique_lock lock(mutex_);
	user_->balance = data.amount;
}

std::optional<SharedConfig> Lobby::readSharedConfig() {
	HANDLE hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, "Local\\CoinPokerWatchdogConfig");
	if (!hMapFile) return std::nullopt;

	void* pBuf = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(SharedConfig));
	if (!pBuf) {
		CloseHandle(hMapFile);
		return std::nullopt;
	}

	SharedConfig data = {};
	memcpy(&data, pBuf, sizeof(SharedConfig));

	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	return data;
}



}
}
