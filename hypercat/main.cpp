#define WIN32_LEAN_AND_MEAN
#undef UNICODE
#include <windows.h>
//#include <WinSock2.h>
#include <WS2tcpip.h>
#include <tlhelp32.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

typedef struct _run_args
{
    PCSTR port;
    BOOL attach;
    BOOL open;
    PSTR file;
    PSTR procname;
    BOOL help;
} run_args;

_Success_(return)
int GetProcessEntry(_In_ const char* filter, _Out_ PPROCESSENTRY32 pProcEntry);
int WriteDataToBuffer(HANDLE hConsoleInput, _In_ const void* data, size_t size);
_Success_(return)
int InitServer(_In_ PCSTR port, _Out_ SOCKET * socket);
_Success_(return)
int ProcessArgs(int argc, char* argv[], _Out_ run_args* args);
void ShowUsage();

int main(int argc, char* argv[])
{
    DWORD dwPID = 0;
    HANDLE hConsoleInput;
    run_args args;

    SOCKET client;
    char recvBuff[1024];

    if (!ProcessArgs(argc, argv, &args))
    {
        ShowUsage();
        exit(EXIT_FAILURE);
    }
    if (args.help)
        exit(NOERROR);

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

    printf_s("Waiting connection...\n");
    if (!InitServer(args.port, &client))
        exit(EXIT_FAILURE);

    FreeConsole();
    AttachConsole(dwPID);
    hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);

    // Receive and writes the data to the target process
    int RecvResult;
    do
    {
        RecvResult = recv(client, recvBuff, sizeof(recvBuff), 0);
        if(RecvResult > 0)
        {
            if(!WriteDataToBuffer(hConsoleInput, recvBuff, RecvResult))
            {
                // Sends the error message to the client
                DWORD msg = GetLastError();
                char sysmsg[256];

                DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, msg,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        sysmsg, 256, NULL);

                send(client, sysmsg, len, 0);
                shutdown(client, SD_SEND);
                closesocket(client);
                WSACleanup();
                CloseHandle(hConsoleInput);
                return FALSE;
            }
        }

    }while(RecvResult > 0);

    // Close the communication of everything occurs well
    if(shutdown(client, SD_SEND) == SOCKET_ERROR)
    {
        perror("shutdown failed!");
        closesocket(client);
        WSACleanup();
        CloseHandle(hConsoleInput);
        return FALSE;
    }

    // Cleaning
    CloseHandle(hConsoleInput);
    closesocket(client);
    WSACleanup();
    return 0;
}


_Success_(return)
int InitServer(_In_ PCSTR port, _Out_ SOCKET * pSocket)
{
    if (!pSocket)
        return FALSE;

    SOCKET ListenSocket;
    struct addrinfo addr;
    PADDRINFOA result = NULL;
    WSADATA wsadata;
    
    if(WSAStartup(MAKEWORD(2,2), &wsadata) != 0)
    {
        perror("WSAStartup failed!\n");
        return FALSE;
    }

    ZeroMemory(&addr, sizeof(struct addrinfo));
    addr.ai_family = AF_INET;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = IPPROTO_TCP;
    addr.ai_flags = AI_PASSIVE;


    if(getaddrinfo(NULL, port, &addr, &result) != 0)
    {
        perror("getaddrinfo failed!\n");
        return FALSE;
    }

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(ListenSocket == INVALID_SOCKET)
    {
        perror("Socket failed!\n");
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return FALSE;
    }

    if(bind(ListenSocket, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
    {
        perror("Bind failed!\n");
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return FALSE;
    }

    freeaddrinfo(result);

    if(listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        perror("listen failed!\n");
        closesocket(ListenSocket);
        WSACleanup();
        return FALSE;
    }

    *pSocket = accept(ListenSocket, NULL, NULL);
    if(*pSocket == INVALID_SOCKET)
    {
        perror("accept failed!\n");
        closesocket(ListenSocket);
        WSACleanup();
        return FALSE;
    }

    closesocket(ListenSocket);

    return TRUE;
}

_Success_(return)
int GetProcessEntry(_In_ const char* filter, _Out_ PPROCESSENTRY32 pProcEntry)
{
    if (!pProcEntry)
        return FALSE;
     
    HANDLE hProcessSnapshot = INVALID_HANDLE_VALUE;

    hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hProcessSnapshot == INVALID_HANDLE_VALUE )
    {
        perror("CreateToolhelp32Snapshot failed!\n");
        return FALSE;
    }
    
    ZeroMemory(pProcEntry, sizeof(PROCESSENTRY32));
    pProcEntry->dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnapshot, pProcEntry))
    {
        perror("Process32First failed!\n");
        CloseHandle(hProcessSnapshot);
        return FALSE;
    }
    else
    {
        do
        {
            if(strcmp(pProcEntry->szExeFile, filter) == 0)
                break;
        } while (Process32Next(hProcessSnapshot, pProcEntry));
    }

    CloseHandle(hProcessSnapshot);
    if(pProcEntry == NULL)
        return FALSE;
    else
        return TRUE;
}

int WriteDataToBuffer(HANDLE hConsoleInput, _In_ const void* data, size_t size)
{
    if (!hConsoleInput || hConsoleInput == INVALID_HANDLE_VALUE)
        return FALSE;
    INPUT_RECORD inRec;
    DWORD EventsWritten;
    BOOL result;
    ZeroMemory(&inRec, sizeof(inRec));
    inRec.EventType = KEY_EVENT;
    inRec.Event.KeyEvent.bKeyDown = TRUE;

    for (int i = 0; i < size; i++)
    {
        inRec.Event.KeyEvent.uChar.AsciiChar = ((char*)data)[i];
        result = WriteConsoleInputA(hConsoleInput, &inRec, 1, &EventsWritten);
        if (!result && EventsWritten != 1)
            return FALSE;
    }
    inRec.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
    inRec.Event.KeyEvent.uChar.AsciiChar = '\r';
    result = WriteConsoleInputA(hConsoleInput, &inRec, 1, &EventsWritten);
    if (!result && EventsWritten != 1)
        return FALSE;
    return TRUE;
}

_Success_(return)
int ProcessArgs(int argc, char* argv[], _Out_ run_args* args)
{
    if (!args || argc <= 1)
        return FALSE;

    const char* args_list[] = { "-PORT", "-ATTACH", "-OPEN", "-FILE", "-PROCNAME" };
    enum args_e { PORT = 0, ATTACH, OPEN, FILE, PROCNAME };

    for (int i = 1; i < argc; i++)
        for (int j = 0; j < strlen(argv[i]); j++)
            if (argv[i][0] == '-')
                argv[i][j] = static_cast<char>(toupper(argv[i][j]));

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "-HELP") == 0)
        {
            args->help = TRUE;
            ShowUsage();
            return TRUE;
        }
    }
    
    for (int i = 0, count = 0; i < argc; i++, count = 0)
    {
        for (int j = 0; j < 5; j++)
            if (strcmp(argv[i], args_list[j]) == 0)
                for (int k = 0; k < argc; k++)
                    if (strcmp(argv[k], args_list[j]) == 0)
                        count++;

        if (argv[i][0] == '-' && count != 1)
        {
            printf_s("Invalid argument '%s'\n", argv[i]);
            return FALSE;
        }
    }


    ZeroMemory(args, sizeof(run_args));
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], args_list[PORT]) == 0)
        {
            if (++i == argc || argv[i][0] == '-')
            {
                perror("Expected the port before '-PORT'\n");
                printf_s("type -h or -help for help\n");
                return FALSE;
            }
            args->port = (PCSTR)malloc((strlen(argv[i]) * sizeof(char)) + 1);
            memcpy_s((void*)args->port, (strlen(argv[i]) * sizeof(char)) + 1, argv[i], strlen(argv[i]) + 1);
        }
        else if (strcmp(argv[i], args_list[ATTACH]) == 0)
            args->attach = TRUE;
        else if (strcmp(argv[i], args_list[OPEN]) == 0)
            args->open = TRUE;
        else if (strcmp(argv[i], args_list[FILE]) == 0)
        {
            if (++i == argc || argv[i][0] == '-')
            {
                perror("Expected the file path before '-FILE'\n");
                printf_s("type -h or -help for help\n");
                return FALSE;
            }
            args->file = (PSTR)malloc((strlen(argv[i]) * sizeof(char)) + 1);
            memcpy_s((void*)args->file, (strlen(argv[i]) * sizeof(char)) + 1, argv[i], strlen(argv[i]) + 1);
        }
        else if (strcmp(argv[i], args_list[PROCNAME]) == 0)
        {
            if (++i == argc || argv[i][0] == '-')
            {
                perror("Expected the process name before '-PROCNAME'\n");
                printf_s("type -h or -help for help\n");
                return FALSE;
            }
            args->procname = (PSTR)malloc((strlen(argv[i]) * sizeof(char)) + 1);
            memcpy_s((void*)args->procname, (strlen(argv[i]) * sizeof(char)) + 1, argv[i], strlen(argv[i]) + 1);
        }
        else
        {
            printf_s("'%s' was unespected\n", argv[i]);
            printf_s("type -h or -help for help\n");
            return FALSE;
        }
    }

    if (!(args->open ^ args->attach))
    {
        printf_s("You need to specify the type 'open' or 'attach'\n");
        printf_s("type -h or -help for help\n");
        return FALSE;
    }

    return TRUE;
}

void ShowUsage()
{
    printf_s("========== How to use ==========\n");
    printf_s("\nConnection:\n");
    printf_s("-PORT -> Define the listening port\n");
    printf_s("\nType:\n");
    printf_s("-ATTACH -> Attach to an existing process\n");
    printf_s("-OPEN -> Open a process\n");
    printf_s("\nTarget:\n");
    printf_s("-FILE -> Path to the process file (open)\n");
    printf_s("-PROCNAME -> Name of the process (attach)\n");
    printf_s("\n-h or -help -> Show this massage\n");
    printf_s("\n============  // ===============\n");
}