#pragma once
#include <string>
#include "../stream_reader.hpp"
#include <nlohmann/json.hpp>
#include <memory>

namespace coinpoker {
namespace protocol {

class LobbyItem {
public:
	uint32_t tableId{};
	uint32_t serverTag{};
	std::string tableName{};
	uint32_t gameType{};
	std::vector<uint32_t> tableType{};
	std::pair<uint32_t, uint32_t> blinds{};
	uint8_t activePlayers{};
	uint8_t maxPlayers{};
	uint8_t ops{};
	uint8_t waitingPlayers{};
	uint32_t minBuyIn{};
	uint32_t maxBuyIn{};
	std::shared_ptr<char[]> bytes{};
	int len{};

	static LobbyItem parse(StreamReader& reader) {
		LobbyItem item;
		std::streampos pos = reader.pos();
		item.tableId = reader.readUInt32();
		item.serverTag = reader.readUInt32();
		item.tableName = reader.readString();
		int len = reader.readSize();
		for (int i = 0; i < len; i++) {
			item.tableType.push_back(reader.readUInt32());
		}
		item.maxPlayers = reader.readUInt8();
		reader.readString();
		item.activePlayers = reader.readUInt8();
		reader.skip(1);
		item.ops = reader.readUInt32();
		int a2 = reader.readUInt8();
		reader.skip(a2 - 1);
		item.gameType = reader.readUInt8();
		item.waitingPlayers = reader.readUInt8();
		reader.skip(3);
		item.blinds.first = reader.readUInt32() / 256;
		item.blinds.second = reader.readUInt32() / 256;
		item.minBuyIn = reader.readUInt32() / 256;
		item.maxBuyIn = reader.readUInt32() / 256;
		reader.skip(8);
		std::streampos newPos = reader.pos();
		int len2 = newPos - pos;
		reader.seek(pos);
		auto temp = reader.readBytes(len2);
		item.bytes = std::shared_ptr<char[]>(temp.release());
		item.len = static_cast<int>(len2);
		reader.seek(newPos);
		return item;
	}

	nlohmann::json toJson() const {
		return {
			{ "tableId", tableId },
			{ "serverTag", serverTag },
			{ "tableName", tableName },
			{ "gameType", gameType },
			{ "tableType", tableType },
			{ "blinds", { blinds.first, blinds.second } },
			{ "activePlayers", activePlayers },
			{ "maxPlayers", maxPlayers },
			{ "waitingPlayers", waitingPlayers },
			{ "minBuyIn", minBuyIn },
			{ "maxBuyIn", maxBuyIn }
		};
	}
};


class LobbyData {
public:
	uint32_t id;
	std::vector<LobbyItem> items{};

	static LobbyData parse(StreamReader& reader) {
		LobbyData d;
		d.id = reader.readUInt32();
		int len = reader.readSize();
		for (int i = 0; i < len; i++) {
			d.items.emplace_back(LobbyItem::parse(reader));
		}
		return d;
	}

	nlohmann::json toJson() const {
		nlohmann::json jItems = nlohmann::json::array();
		for (const auto& item : items) {
			jItems.push_back(item.toJson());
		}
		return {
			{ "id", id },
			{ "items", jItems }
		};
	}
};

}
}
