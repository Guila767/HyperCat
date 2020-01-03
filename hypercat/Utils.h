#pragma once

#undef UNICODE
#include <Windows.h>
#include <TlHelp32.h>


_Success_(return != 0)
int GetProcessEntry(_In_ const char* filter, _Out_ PPROCESSENTRY32 pProcEntry);

_Success_(return != 0)
int GetErrorMsg(_Outptr_ LPSTR * lpStr);