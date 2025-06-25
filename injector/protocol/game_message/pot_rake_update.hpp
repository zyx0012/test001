#pragma once
#include <vector>
#include <nlohmann/json.hpp>
#include "../../utils/utils.hpp"

namespace coinpoker {
namespace protocol {

class PotRake {
public:
	uint8_t index;
	uint64_t amount;
	uint64_t rake;
	uint64_t finalAmount;

	nlohmann::json toJson() const {
		return {
			{"index", index},
			{"amount", amount},
			{"rake", rake},
			{"finalAmount", finalAmount}
		};
	}
};

class PotRakeUpdate {
public:
	std::vector<PotRake> pots;


	static PotRakeUpdate parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		PotRakeUpdate packet;
		int length = std::stoi(parts[0]);
		for (int i = 0; i < length; i++) {
			PotRake potRake;
			potRake.index = std::stoi(parts[i * 3 + 1]);
			potRake.amount = std::stol(parts[i * 3 + 2]);
			potRake.rake = std::stol(parts[i * 3 + 3]);
			potRake.finalAmount = potRake.amount - potRake.rake;
			packet.pots.push_back(potRake);
		}
		return packet;
	}

	nlohmann::json toJson() const {
		nlohmann::json jsonPots = nlohmann::json::array();
		for (const auto& pot : pots) {
			jsonPots.push_back(pot.toJson());
		}
		return { {"pots", jsonPots} };
	}


};

}
}
