#pragma once

#include <string>
#include <format>

namespace coinpoker {
namespace protocol {

class GetPlayers {
public:
	uint32_t tableId{};
	uint32_t userId{};

	std::string serialize() {
		return std::format("players\t{}\t{}\t{}\t{}", "poker_ring", 305, tableId, userId);
	}
};
}
}
