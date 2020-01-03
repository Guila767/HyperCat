#include "Utils.h"

_Success_(return != 0)
int GetProcessEntry(_In_ const char* filter, _Out_ PPROCESSENTRY32 pProcEntry)
{
    if (!pProcEntry)
        return FALSE;

    HANDLE hProcessSnapshot = INVALID_HANDLE_VALUE;

    hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnapshot == INVALID_HANDLE_VALUE)
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
            if (strcmp(pProcEntry->szExeFile, filter) == 0)
                break;
        } while (Process32Next(hProcessSnapshot, pProcEntry));
    }

    CloseHandle(hProcessSnapshot);
    if (pProcEntry == NULL)
        return FALSE;
    else
        return TRUE;
}

_Success_(return != 0)
int GetErrorMsg(_Outptr_ LPSTR* lpStr)
{
    DWORD msg = GetLastError();
    char buffer[256];

    DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, msg,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buffer, sizeof(buffer), NULL);

    if (!(*lpStr = (LPSTR)malloc((len + 1) * sizeof(char))) || len <= 0)
        return FALSE;
    *lpStr[len + 1] = '\n';
    memcpy_s(*lpStr, len * sizeof(char), buffer, sizeof(buffer));
    return 0;
}