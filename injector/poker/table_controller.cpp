#include "table_controller.hpp"
#include "../utils/config.hpp"
#include "../utils/utils.hpp"
#include "../protocol/game_message/join.hpp"
#include "../protocol/game_message/players.hpp"

namespace coinpoker {


std::unordered_map<int, std::shared_ptr<Table>> TableController::tables = {};
std::shared_mutex TableController::tableMtx = {};


void TableController::onNetworkPacket(std::shared_ptr<Websocket> ws, protocol::GamePacket& gamePacket) {
	int tableId = gamePacket.getInstanceId();
	if (tableId == 0) return;
	if (!gamePacket.isInteresting()) {
		return;
	};
	std::shared_ptr<Table> tablePtr;
	{
		std::shared_lock lock{ tableMtx };
		auto it = tables.find(tableId);
		if (it == tables.end()) return;
		tablePtr = it->second;
	}
	gamePacket.takeOwnership();
	tablePtr->pushPacket(std::move(gamePacket));
}

std::optional<protocol::GamePacketRequest> TableController::onSendNetworkPacket(std::shared_ptr<Websocket> ws, protocol::GamePacketRequest& gamePacket) {
	if (!gamePacket.isInteresting()) return std::nullopt;
	const auto& logger = utils::CreateLoggerNoPrefix("tableDriver");
	if (gamePacket.getType() == "join") {
		onJoin(ws, gamePacket);
		return std::nullopt;
	}
	else if (gamePacket.getType() == "leave") {
		onLeave(ws, gamePacket);
		return std::nullopt;
	}
	else {
		return onSendPacket(gamePacket);
	}
}

std::optional<protocol::GamePacketRequest> TableController::onSendPacket(protocol::GamePacketRequest& gamePacket) {
	int instanceId = gamePacket.getInstanceId();
	std::shared_ptr<Table> tablePtr;
	{
		std::shared_lock lock{ tableMtx };
		auto it = tables.find(instanceId);
		if (it == tables.end()) return std::nullopt;
		tablePtr = it->second;
	}
	return tablePtr->onSendPacket(gamePacket);
}

void TableController::onJoin(std::shared_ptr<Websocket> ws, protocol::GamePacketRequest& gamePacket) {
	const auto& logger = utils::CreateLoggerNoPrefix("tableDriver");
	auto parts = utils::splitString(gamePacket.getBody(), '\t');
	int tableId = std::stoi(parts[1]);
	int userId = std::stoi(parts[2]);
	int instanceId = gamePacket.getInstanceId();
	logger->info("Joining table: instance: {}, id: {}", instanceId, tableId);
	std::shared_ptr<Table> newTable;
	{
		std::unique_lock lock{ tableMtx };
		auto it = tables.find(instanceId);
		if (it == tables.end()) {
			newTable = std::make_shared<Table>(tableId, instanceId, userId, gamePacket.getHeader().operationId, gamePacket.getHeader().serverTarget, ws);
			tables[instanceId] = newTable;
		}
		else {
			it->second->setWebSocket(ws);
		}
	}
	if (newTable) {
		newTable->start();
	}
}

void TableController::onLeave(std::shared_ptr<Websocket> ws, protocol::GamePacketRequest& gamePacket) {
	const auto& logger = utils::CreateLoggerNoPrefix("tableDriver");
	int instanceId = gamePacket.getInstanceId();
	auto parts = utils::splitString(gamePacket.getBody(), '\t');
	int tableId = std::stoi(parts[1]);
	logger->info("Leaving table: instance: {}, id: {}", instanceId, tableId);
	std::shared_ptr<Table> table;
	{
		std::unique_lock lock{ tableMtx };
		auto it = tables.find(instanceId);
		if (it == tables.end()) return;
		table = it->second;
		tables.erase(it);
	}
	Kill(table);
}

void TableController::Kill(std::shared_ptr<Table> table) {
	if (!table) return;

	{
		std::lock_guard lock(table->mtx);
		table->isStopped = true;
	}
	table->cv.notify_one();

	if (table->consumerThread.joinable()) {
		table->consumerThread.join();
	}
}

void TableController::SitOutAll() {
	std::unique_lock lock{ tableMtx };
	for (auto& table : tables) {
		table.second->sitOut();
	}
}

void TableController::SitOut(uint32_t tableId) {
	std::unique_lock lock{ tableMtx };
	for (auto& [_, table] : tables) {
		if (table->getId() == tableId) {
			table->sitOut();
			return;
		}
	}
}

uint32_t TableController::getPlayingBalance() {
	std::shared_lock lock{ tableMtx };
	uint64_t balance = 0;
	for (auto& table : tables) {
		balance += table.second->getHeroStack();
	}
	return static_cast<uint32_t>((balance + 50) / 100);
}

}
