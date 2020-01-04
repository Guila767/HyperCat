#pragma once
#include <Windows.h>

class HyperCat
{
private:
	HANDLE hConsoleInput;
	PCSTR port;
	DWORD processID;

	static int WriteDataToBuffer(HANDLE hConsoleInput, _In_ const void* data, size_t size);
	_Success_(return != 0)
	static int InitServer(_In_ PCSTR port, _Out_ SOCKET * socket);
	void CleanUp(SOCKET socket);

public:
	HyperCat(_In_ PCSTR port, _In_ DWORD dwProcessID);
	~HyperCat();
	bool Begin();
};
