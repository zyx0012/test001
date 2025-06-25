#pragma once
#include <Windows.h>

namespace coinpoker {
namespace hook {

	void startInjection(HMODULE dllInstance);
	void disableHooks();
}
}
