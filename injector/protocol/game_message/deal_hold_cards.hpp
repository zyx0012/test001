#pragma once
#include <string>
#include "../../utils/utils.hpp"
#include <nlohmann/json.hpp>

namespace coinpoker {
namespace protocol {

class DealHoleCards {

public:

	uint8_t tableSize{};
	std::string status{};
	uint8_t type{};
	uint8_t nrCards{};
	std::string dealtTo{};

	static DealHoleCards parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		DealHoleCards packet;
		packet.tableSize = std::stoi(parts[0]);
		packet.status = parts[1];
		packet.type = std::stoi(parts[2]);
		packet.nrCards = std::stoi(parts[3]);
		packet.dealtTo = parts[4];
		return packet;
	}

	nlohmann::json toJson() const {
		return {
			{"tableSize", tableSize},
			{"status", status},
			{"type", type},
			{"nrCards", nrCards},
			{"dealtTo", dealtTo},
		};
	}
};
};
};
