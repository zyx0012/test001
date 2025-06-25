#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace coinpoker {
namespace protocol {
class EndGame {
public:
	std::string summary{};

	static EndGame parse(std::string body) {
		EndGame packet;
		packet.summary = body;
		return packet;
	}

	nlohmann::json toJson() const {
		return {
			{"summary", summary}
		};
	}
};
}
}
