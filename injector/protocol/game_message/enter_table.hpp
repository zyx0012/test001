#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "../../utils/utils.hpp"


namespace coinpoker {
namespace protocol {

class EnterTable {

public:
	uint8_t seatIndex{};
	std::string name{};

	static EnterTable parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		EnterTable packet;
		packet.seatIndex = std::stoi(parts[0]);
		packet.name = parts[2];
		return packet;
	}

	[[nodiscard]] nlohmann::json toJson() const {
		return {
			{"seatIndex", seatIndex},
			{"name", name}
		};
	}
};

}
}
