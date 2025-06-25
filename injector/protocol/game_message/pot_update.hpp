#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "../../utils/utils.hpp"
#include <vector>


namespace coinpoker {
namespace protocol {

class PotAction {
public:
	std::string fromType;
	uint8_t fromPosition;
	uint64_t totalAmountFrom;
	uint64_t amount;
	std::string toType;
	uint8_t toPosition;
	uint64_t totalAmountTo;
	std::string actionType;
	std::string handResult;


	nlohmann::json toJson() const {
		return nlohmann::json{
			{"fromType", fromType},
			{"fromPosition", fromPosition},
			{"totalAmountFrom", totalAmountFrom},
			{"amount", amount},
			{"toType", toType},
			{"toPosition", toPosition},
			{"totalAmountTo", totalAmountTo},
			{"actionType", actionType},
			{"handResult", handResult}
		};
	}
};

class PotUpdate {

public:
	std::vector<PotAction> pots;

	static PotUpdate parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		PotUpdate packet;

		int length = std::stoi(parts[0]);
		for (int i = 0; i < length; i++) {
			PotAction data;
			data.fromType = parts[i * 9 + 1];
			data.fromPosition = std::stoi(parts[i * 9 + 2]);
			data.totalAmountFrom = std::stol(parts[i * 9 + 3]);
			data.amount = std::stol(parts[i * 9 + 4]);
			data.toType = parts[i * 9 + 5];
			data.toPosition = std::stoi(parts[i * 9 + 6]);
			data.totalAmountTo = std::stol(parts[i * 9 + 7]);
			data.actionType = parts[i * 9 + 8];
			data.handResult = parts[i * 9 + 9];
			packet.pots.push_back(data);
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
