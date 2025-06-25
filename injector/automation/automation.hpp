#pragma once
#include <iostream>
#include <memory>
#include "../hooks/internal_functions.hpp"

namespace coinpoker {
namespace automation {
class Automation {
public:
	static void setInternalFunctions(std::shared_ptr<IInternalFunctions> qtFunctions);
	static int onGuiStart();

private:
	static std::shared_ptr<IInternalFunctions> internalFunctions_;
};
}
}
