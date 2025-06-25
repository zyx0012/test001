#pragma once

#include <vector>
#include <nlohmann/json.hpp>
#include "../../utils/utils.hpp"

namespace coinpoker {
namespace protocol {
class BuyInPacket {
public:

	uint32_t opId{};
	uint32_t tableId{};
	std::string type{};
	uint32_t minBuyIn{};
	uint32_t maxBuyIn{};

	static BuyInPacket parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		BuyInPacket p;
		p.opId = std::stoi(parts[1]);
		p.tableId = std::stoi(parts[2]);
		p.type = parts[8];
		p.minBuyIn = std::stoi(parts[10]);
		p.maxBuyIn = std::stoi(parts[11]);
		return p;
	}

	nlohmann::json toJson() const {
		return {
			{"opId", opId},
			{"tableId", tableId},
			{"type", type},
			{"minBuyIn", minBuyIn},
			{"maxBuyIn", maxBuyIn}
		};
	}

};
};
}
