#pragma once
#include "../protocol/game_packet.hpp" 
#include "table.hpp"
#include "../hooks/websocket.hpp"
#include <map>
#include <memory>
#include <shared_mutex>
#include "../utils/logging.hpp"

namespace coinpoker {
class TableController {

public:
	static void onNetworkPacket(std::shared_ptr<Websocket> ws, protocol::GamePacket& packet);
	static std::optional<protocol::GamePacketRequest> onSendNetworkPacket(std::shared_ptr<Websocket> ws, protocol::GamePacketRequest& packet);
	static void SitOutAll();
	static void SitOut(uint32_t tableId);
	static uint32_t getPlayingBalance();

private:
	static void onJoin(std::shared_ptr<Websocket> ws, protocol::GamePacketRequest& packet);
	static void onLeave(std::shared_ptr<Websocket> ws, protocol::GamePacketRequest& packet);
	static std::optional<protocol::GamePacketRequest> onSendPacket(protocol::GamePacketRequest& packet);
	static void Kill(std::shared_ptr<Table> table);
	static std::unordered_map<int, std::shared_ptr<Table>> tables;
	static std::shared_mutex  tableMtx;
};
}
