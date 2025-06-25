#pragma once
#include <vector>
#include <nlohmann/json.hpp>
#include "../../utils/utils.hpp"

namespace coinpoker {
namespace protocol {
class HeroTurn {
public:

	uint8_t seat{};
	std::string type{};
	uint32_t stack{};
	uint32_t commitedAmount{};
	uint32_t cap{};
	uint32_t amountToCall{};
	uint32_t mniRaise{};
	uint32_t maxRaise{};
	uint32_t pot{};

	static HeroTurn parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		HeroTurn packet;
		packet.seat = std::stoi(parts[0]);
		packet.type = parts[1];
		if (packet.type == "betting") {
			packet.stack = std::stoi(parts[2]);
			packet.commitedAmount = std::stoi(parts[3]);
			packet.cap = std::stoi(parts[4]);
			packet.amountToCall = std::stoi(parts[5]);
			packet.mniRaise = std::stoi(parts[6]);
			packet.maxRaise = std::stoi(parts[7]);
		}
		return packet;
	}

	nlohmann::json toJson() const {
		return {
			{"seat", seat},
			{"type", type},
			{"stack", stack},
			{"commitedAmount", commitedAmount},
			{"cap", cap},
			{"amountToCall", amountToCall},
			{"mniRaise", mniRaise},
			{"maxRaise", maxRaise},
			{"pot", pot}
		};
	}

};
};
}
