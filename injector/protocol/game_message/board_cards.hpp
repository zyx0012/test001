#pragma once
#include <string>
#include "../../utils/utils.hpp"
#include <nlohmann/json.hpp>


namespace coinpoker {
namespace protocol {

class BoardCards {

public:
	uint32_t a{};
	uint32_t startPosition{};
	uint32_t position{};
	std::string cards{};

	static BoardCards parse(std::string body) {
		auto parts = utils::splitString(body, '\t');
		BoardCards packet;

		packet.a = std::stoi(parts[0]);
		packet.startPosition = std::stoi(parts[1]);
		packet.position = std::stoi(parts[2]);
		packet.cards = parts[3];
		return packet;
	}

	[[nodiscard]] nlohmann::json toJson() const {
		return {
			{"a", a},
			{"startPosition", startPosition},
			{"position", position},
			{"cards", cards}
		};
	}
};
};
}

