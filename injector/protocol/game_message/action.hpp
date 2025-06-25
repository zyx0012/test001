#pragma once
#include <string>
#include "../../utils/utils.hpp"
#include <nlohmann/json.hpp>


namespace coinpoker {
namespace protocol {

class Action {

public:
	int32_t seatIndex{};
	int64_t stack{};
	int64_t commitedAmount{};
	int64_t a1{};
	int64_t amount{};
	std::string action{};
	std::string showCards{};
	std::string msg{};
	int64_t a6{};
	int64_t amountToCall{};

	static Action parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		Action packet;

		packet.seatIndex = std::stoi(parts[0]);
		packet.stack = std::stol(parts[1]);
		packet.commitedAmount = std::stol(parts[2]);
		packet.a1 = std::stol(parts[3]);
		packet.action = parts[5];
		if (packet.action == "H") {
			packet.showCards = parts[4];
		}
		else {
			packet.amount = stol(parts[4]);
		}
		packet.msg = parts[6];
		packet.a6 = std::stoi(parts[7]);
		packet.amountToCall = std::stoi(parts[8]);
		return packet;
	}

	[[nodiscard]] nlohmann::json toJson() const {
		return {
			{"seatIndex", seatIndex},
			{"stack", stack},
			{"commitedAmount", commitedAmount},
			{"a1", a1},
			{"amount", amount},
			{"action", action},
			{"showcards", showCards},
			{"msg", msg},
			{"a6", a6},
			{"amountToCall", amountToCall}
		};
	}
};
};
};
