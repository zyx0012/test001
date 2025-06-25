#pragma once

#include <string>
#include "../../utils/utils.hpp"
#include <nlohmann/json.hpp>

namespace coinpoker {
namespace protocol {

class DealHeroCards {

public:
	uint8_t seatIndex{};
	uint8_t nrCards{};
	std::string cards{};

	static DealHeroCards parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		DealHeroCards packet;
		packet.seatIndex = std::stoi(parts[0]);
		packet.nrCards = std::stoi(parts[1]);
		packet.cards = parts[2];
		return packet;
	}

	nlohmann::json toJson() const {
		return {

			{"seatIndex", seatIndex},
			{"nrCards", nrCards},
			{"cards", cards},
		};
	}
};
};
};
