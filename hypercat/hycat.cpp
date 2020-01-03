#define WIN32_LEAN_AND_MEAN
#undef UNICODE

#include <WS2tcpip.h>
#include "hycat.h"
#include <tlhelp32.h>
#include <stdlib.h>
#include "Utils.h"

static void strcpy_a(PSTR* dst, PCSTR const src);

_Success_(return != 0)
int HyperCat::InitServer(_In_ PCSTR port, _Out_ SOCKET * pSocket)
{
    if (!pSocket)
        return FALSE;

    SOCKET listenSocket;
    struct addrinfo addr;
    PADDRINFOA result = NULL;
    WSADATA wsadata;

    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
    {
        perror("WSAStartup failed!\n");
        return FALSE;
    }

    ZeroMemory(&addr, sizeof(struct addrinfo));
    addr.ai_family = AF_INET;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = IPPROTO_TCP;
    addr.ai_flags = AI_PASSIVE;


    if (getaddrinfo(NULL, port, &addr, &result) != 0)
    {
        perror("getaddrinfo failed!\n");
        return FALSE;
    }

    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET)
    {
        perror("Socket failed!\n");
        freeaddrinfo(result);
        closesocket(listenSocket);
        WSACleanup();
        return FALSE;
    }

    if (bind(listenSocket, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
    {
        perror("Bind failed!\n");
        freeaddrinfo(result);
        closesocket(listenSocket);
        WSACleanup();
        return FALSE;
    }

    freeaddrinfo(result);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        perror("listen failed!\n");
        closesocket(listenSocket);
        WSACleanup();
        return FALSE;
    }

    *pSocket = accept(listenSocket, NULL, NULL);
    if (*pSocket == INVALID_SOCKET)
    {
        perror("accept failed!\n");
        closesocket(listenSocket);
        WSACleanup();
        return FALSE;
    }

    closesocket(listenSocket);

    return TRUE;
}

void HyperCat::CleanUp(SOCKET _socket)
{
    shutdown(_socket, SD_SEND);
    closesocket(_socket);
    CloseHandle(this->hConsoleInput);
    this->hConsoleInput = INVALID_HANDLE_VALUE;
}

HyperCat::HyperCat(_In_ PCSTR port, _In_ DWORD dwProcessID) : processID(dwProcessID), hConsoleInput(INVALID_HANDLE_VALUE)
{
    strcpy_a((PSTR*)&this->port, port);
}

HyperCat::~HyperCat()
{
    free((void*)port);
    WSACleanup();
}

bool HyperCat::Begin()
{
    SOCKET client;
    if (!InitServer(this->port, &client))
    {
        perror("InitServer failed");
        return false;
    }

    FreeConsole();
    if (!AttachConsole(processID))
    {
        perror("AttachConsole failed!\n");
        return false;
    }
    hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    
    int RecvResult;
    char recvBuff[1024];
    do
    {
        RecvResult = recv(client, recvBuff, sizeof(recvBuff), 0);
        if (RecvResult > 0)
        {
            if (!WriteDataToBuffer(hConsoleInput, recvBuff, RecvResult))
            {
                LPSTR lpMsg;
                if (GetErrorMsg(&lpMsg))
                {
                    send(client, lpMsg, strlen(lpMsg), 0);
                    free(lpMsg);
                }
                CleanUp(client);
                return false;
            }
        }

    } while (RecvResult > 0);
    CleanUp(client);
    return true;
}

int HyperCat::WriteDataToBuffer(HANDLE hConsoleInput, _In_ const void* data, size_t size)
{
    if (!hConsoleInput || hConsoleInput == INVALID_HANDLE_VALUE)
        return FALSE;
    INPUT_RECORD inRec;
    DWORD EventsWritten;
    BOOL result;
    ZeroMemory(&inRec, sizeof(inRec));
    inRec.EventType = KEY_EVENT;
    inRec.Event.KeyEvent.bKeyDown = TRUE;

    for (size_t i = 0; i < size; i++)
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


void strcpy_a(PSTR* dst, PCSTR const src)
{
    size_t size = sizeof(char) * (strlen(src) + 1);
    *dst = static_cast<PSTR>(malloc(size));
    memcpy_s(dst, size, src, size);
}
