#include "./hooks/hook.hpp"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        coinpoker::hook::startInjection(hModule);
        break;
    case DLL_PROCESS_DETACH:
        coinpoker::hook::disableHooks();
        break;
    }
    return TRUE;
}

