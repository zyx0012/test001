#pragma once
#include "../../utils/utils.hpp"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace coinpoker {
namespace protocol {
class Seat {

public:
	uint8_t index{};
	std::string status;
	std::string name;
	std::string avatar;
	uint64_t stack;
	std::string bounty;
	std::string playerId;
	std::string platform;

	nlohmann::json toJson() const {

		nlohmann::json::object_t result = {
			{"index", index},
			{"status", status},
			{"name", name},
			{"avatar", avatar},
			{"stack", stack},
			{"bounty", bounty},
			{"playerId", playerId},
			{"platform", platform}
		};
		return result;
	}
};


class TableState {

public:
	std::string name{};
	uint8_t tableSize{};
	std::string currency{};
	uint64_t ante{};
	uint64_t smallBlind{};
	uint64_t bigBlind{};
	uint64_t stakes{};
	std::string gameType{};
	std::string potType{};
	uint32_t tour{};
	std::string state{};
	uint8_t activePlayers{};
	std::string handId{};
	uint8_t dealerPos{};

	std::vector<Seat> seats;

	static TableState parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		TableState packet;

		packet.name = parts[0];
		packet.tableSize = std::stoi(parts[1]);
		packet.currency = parts[2];
		packet.ante = std::stoul(parts[4]);
		packet.smallBlind = std::stoul(parts[5]);
		packet.bigBlind = std::stoul(parts[6]);
		packet.stakes = std::stoul(parts[7]);
		packet.gameType = parts[8];
		packet.potType = parts[9];
		packet.tour = std::stoi(parts[11]);
		packet.state = parts[16];
		packet.activePlayers = std::stoi(parts[22]);
		packet.handId = parts[23];
		packet.dealerPos = std::stoi(parts[24]);
		if (parts[27] == "seats") {
			int seats = std::stoi(parts[28]);
			int index = 29;
			for (int i = 0; i < seats; i++) {
				Seat seat;
				seat.index = std::stoi(parts[index++]);
				seat.status = parts[index++];
				seat.name = parts[index++];
				seat.avatar = parts[index++];
				seat.stack = std::stoi(parts[index++]);
				index++;
				seat.bounty = parts[index++];
				index += 2;
				seat.playerId = parts[index++];
				seat.platform = parts[index++];
				packet.seats.push_back(seat);
			}
		}
		return packet;
	}

	[[nodiscard]] nlohmann::json toJson() const {

		nlohmann::json seatsJson = nlohmann::json::array();
		for (const auto& seat : seats) {
			seatsJson.push_back(seat.toJson());
		}

		nlohmann::json::object_t result = {
			{"name", name},
			{"tableSize", tableSize},
			{"currency", currency},
			{"ante", ante},
			{"smallBlind", smallBlind},
			{"bigBlind", bigBlind},
			{"stakes", stakes},
			{"gameType", gameType},
			{"tour", tour},
			{"state", state},
			{"activePlayers", activePlayers},
			{"handId", handId},
			{"dealerPos", dealerPos},
			{"seats", seatsJson},
		};
		return result;
	}
};
};
};
