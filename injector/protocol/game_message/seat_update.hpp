#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "../../utils/utils.hpp"

namespace coinpoker {
namespace protocol {
class SeatUpdate {
public:
	uint8_t index{};
	std::string status{};
	std::string name{};
	std::string avatar{};
	uint64_t stack{};
	std::string bounty{};
	std::string playerId{};
	std::string platform{};


	static SeatUpdate parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		SeatUpdate packet;
		packet.index = std::stoi(parts[0]);
		packet.status = parts[1];
		packet.name = parts[2];
		packet.avatar = parts[3];
		packet.stack = std::stol(parts[4]);
		packet.bounty = parts[6];
		packet.playerId = parts[9];
		packet.platform = parts[10];
		return packet;
	}

	nlohmann::json toJson() const {
		return {
			{"index", index},
			{"status", status},
			{"name", name},
			{"avatar", avatar},
			{"stack", stack},
			{"bounty", bounty},
			{"playerId", playerId},
			{"platform", platform}
		};
	}
};
}
}
