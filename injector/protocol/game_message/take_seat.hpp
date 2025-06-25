#pragma once
#include <string>
#include <format>

namespace coinpoker {
namespace protocol {

class TakeSeat {
public:
	uint32_t tableId{};
	uint32_t userId{};
	uint32_t tableSeat{};

	std::string serialize() {
		return std::format("seat\t{}\t{}\t{}\t{}",tableId, userId, tableSeat, 1);
	}
};

}
}
