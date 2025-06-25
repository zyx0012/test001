#pragma once
#include <string>
#include <format>

namespace coinpoker {
namespace protocol {
class Join {
public:
	uint32_t tableId{};
	uint32_t userId{};

	std::string serialize() {
		return std::format("join\t{}\t{}\t{}", tableId, userId, -1);
	}
};
}
}
