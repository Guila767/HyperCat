#define WIN32_LEAN_AND_MEAN
#undef UNICODE

#include <windows.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "RunArgs.h"
#include "Utils.h"
#include "hycat.h"

void ShowUsage();

int main(int argc, char* argv[])
{ 
    DWORD dwPID = 0;
    run_args args;

    if (!ProcessArgs(argc, argv, &args))
        if(argc > 1)
            exit(EXIT_FAILURE);
    if (args.help || argc <= 1)
    {
        ShowUsage();
        exit(NOERROR);
    }

    // Get the PID
    if (args.open)
    {
        
        STARTUPINFOA startInfo;
        ZeroMemory(&startInfo, sizeof(STARTUPINFOA));
        startInfo.cb = sizeof(STARTUPINFOA);

        PROCESS_INFORMATION procInfo;
        if (!CreateProcessA(args.file, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startInfo, &procInfo))
        {
            perror("CreateProcess Failed!\n");
            return EXIT_FAILURE;
        }
        dwPID = procInfo.dwProcessId;
        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);
    }
    else
    {
        PROCESSENTRY32 pe32;
        if (!GetProcessEntry(args.procname, &pe32))
        {
            perror("Get process entry failed!\n");
            return EXIT_FAILURE;
        }
        dwPID = pe32.th32ProcessID;
    }

    HyperCat hycat(args.port, dwPID);
    printf_s("Waiting connection on port '%s'...\n", args.port);
    if (!hycat.Begin())
        return EXIT_FAILURE;
    FreeRunArgs(&args);
    return 0;
}

void ShowUsage()
{
    printf_s("\n========== How to use ==========\n");
    printf_s("\nConnection:\n");
    printf_s("-PORT -> Define the listening port\n");
    printf_s("\nType:\n");
    printf_s("-ATTACH -> Attach to an existing process\n");
    printf_s("-OPEN -> Open a process\n");
    printf_s("\nTarget:\n");
    printf_s("-FILE -> Path to the executable file (open)\n");
    printf_s("-PROCNAME -> Name of the process (attach)\n");
    printf_s("\n-h or -help -> Show this massage\n");
    printf_s("\n============  // ===============\n");
}
