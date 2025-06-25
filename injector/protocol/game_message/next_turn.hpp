#pragma once
#include <vector>
#include <nlohmann/json.hpp>
#include "../../utils/utils.hpp"

namespace coinpoker {
namespace protocol {
class NextTurn {
public:

	uint8_t seat;
	uint32_t timeTotal;
	uint32_t time;
	uint32_t waitCount;

	static NextTurn parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		NextTurn packet;
		packet.seat = std::stoi(parts[0]);
		packet.timeTotal = std::stoi(parts[1]);
		packet.time = std::stoi(parts[2]);
		packet.waitCount = std::stoi(parts[3]);
		return packet;
	}

	 nlohmann::json toJson() const {
	return {
		{"seat", seat},
		{"timeTotal", timeTotal},
		{"time", time},
		{"waitCount", waitCount}
	};
}

};
}
}
