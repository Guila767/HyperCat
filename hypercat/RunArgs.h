#pragma once
#include <Windows.h>

typedef struct _run_args
{
    PCSTR port;
    BOOL attach;
    BOOL open;
    PSTR file;
    PSTR procname;
    BOOL help;
} run_args;

_Success_(return != 0)
// Process the arguments and write to a run_args structure.
// The function will allocate resources in the run_args structure that needs to be freed.
int ProcessArgs(int argc, char* argv[], _Out_ run_args * args);

void FreeRunArgs(run_args* pArgs);
