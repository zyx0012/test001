#pragma once
#include <string>
#include <vector>
#include "QtCore/qcoreapplication.h"
#include "QtCore/qtimer.h"
#include <QtCore/QMetaMethod>
#include "QtWidgets/qwidget.h"
#include "QtWidgets/qlineedit.h"
#include "QtWidgets/qpushbutton.h"
#include "QtWidgets/qtableview.h"
#include "../utils/config.hpp"
#include "../protocol/stream_reader.hpp"
#include "../protocol/game_message/lobby_data.hpp"
#include "../protocol/game_message/login_response.hpp"
#include <regex>
#include <map>
#include <unordered_set>
#include "QtWidgets/qtableview.h"
#include <queue>
#include <thread>
#include "../poker/table_controller.hpp"
#include "QtWidgets/qapplication.h"
#include "./poker_table.hpp"
#include "../utils/logging.hpp"
#include "./lobby_types.hpp"

namespace coinpoker {

namespace automation {


struct SharedConfig {
	int64_t sessionStartTime;
};

struct CurrentTable {
	uint32_t id{};
	int configIndex = -1;
	PokerTableWrapper tableWidget;
	uint32_t gameType{};
	std::vector<uint32_t> tableType{};
	std::pair<uint32_t, uint32_t> blinds{};
	uint8_t maxPlayers{};
	CurrentState state{};
};

class Lobby {
	using clock = std::chrono::system_clock;
public:
	static Lobby& instance();

	void closePokerWindow(uint32_t tableId, CloseReason reason);
	void addPokerTable(QWidget* widget);
	void onLobbyPacket(std::shared_ptr<Websocket> ws, int msgId, protocol::StreamReader& reader);
	void onBudgetChange(protocol::GamePacket& packet);
	void updateLobbyItems(protocol::StreamReader& reader);
	void consumerThread();
	void reportBalance();
	void start();
	void setLobbyWidget(QTableView* w);
	void openTable(std::shared_ptr<CurrentTable> item);
	void removePokerTable(QWidget* widget);
	void changeTableState(uint32_t tableId, CurrentState state);
	void checkLoneTables(int index);
	bool shouldBuyIn(uint32_t tableId);
	void endSession();
	std::optional<UserData> getUser();

	std::map<uint32_t, protocol::LobbyItem> lobbyItems;
	std::map<uint32_t, std::shared_ptr<CurrentTable>> currentTables;
	std::map<uint32_t, clock::time_point> cooldownTables;

	clock::time_point lastTimestamp;
	std::atomic_bool isStopped{ false };

private:
	Lobby();
	~Lobby();

	Lobby(const Lobby&) = delete;
	Lobby& operator=(const Lobby&) = delete;
	Lobby(Lobby&&) = delete;
	Lobby& operator=(Lobby&&) = delete;

	void run();
	void configRun(utils::TableConfig& config, int index);
	bool match(const protocol::LobbyItem& item, utils::TableConfig& config);
	void doClosePokerWindow();
	void doSitOut();
	bool isBreakTime(std::chrono::minutes time);
	std::optional<SharedConfig> readSharedConfig();
	QTableView* lobbyTable_ = nullptr;
	std::mutex mutex_;
	clock::time_point createdAt_;
	clock::time_point sessionStartedAt_;
	std::condition_variable cond_;
	std::atomic_bool isSessionStopped{ false };
	std::atomic_bool isBreak{ false };
	std::queue<protocol::StreamReader> packetQueue_;
	std::thread workerThread_;
	std::shared_ptr<Websocket> ws;
	std::optional<UserData> user_;
	utils::Logger logger;
	std::vector<std::chrono::minutes> breaksDuration_;
	int breaks_;
	int sessionLength_;
};

class LobbyListHandler : public WidgetHandler {
	bool keyLoggerInstalled = false;
public:
	bool matches(QWidget* w, QEvent* event) const override;
	void handle(QWidget* w, QEvent* event) override;

};


}


}
