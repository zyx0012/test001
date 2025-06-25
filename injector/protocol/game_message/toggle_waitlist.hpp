#pragma once
#include <string>
#include <format>

namespace coinpoker {
namespace protocol {

class ToggleWaitList {
public:
	uint32_t tableId{};
	uint32_t userId{};
	bool value{};

	std::string serialize() {
		return std::format("waiting\t{}\t{}\t{}",tableId, userId,static_cast<int>(value));
	}
};
}
}
