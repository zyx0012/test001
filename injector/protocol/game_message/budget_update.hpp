#pragma once
#include <string>
#include "../../utils/utils.hpp"

namespace coinpoker {
namespace protocol {

class BudgetUpdate {
public:
	uint32_t amount;
	std::string  currency;

	static BudgetUpdate parse(const std::string& body) {
		BudgetUpdate p;
		const auto& parts = utils::splitString(body, '\t');
		p.amount = std::stoi(parts[1]);
		return p;
	}
};

}
}
