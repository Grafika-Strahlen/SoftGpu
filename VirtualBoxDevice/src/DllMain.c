#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
    (void) hInstDll;

    // Perform actions based on the reason for calling.
    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
             // Initialize once for each new process.
             // Return FALSE to fail DLL load.
            break;

        case DLL_THREAD_ATTACH:
            // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
            // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
            if(lpvReserved)
            {
                break; // do not do cleanup if process termination scenario
            }

            // Perform any necessary cleanup.
            break;
        default: break;
    }

    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

#ifdef __cplusplus
}
#endif
