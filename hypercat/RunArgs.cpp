#include "RunArgs.h"
#include <stdio.h>

_Success_(return != 0)
int ProcessArgs(int argc, char* argv[], _Out_ run_args * args)
{
    if (!args || argc <= 1)
        return FALSE;

    const char* args_list[] = { "-PORT", "-ATTACH", "-OPEN", "-FILE", "-PROCNAME" };
    enum args_e { PORT = 0, ATTACH, OPEN, FILE, PROCNAME };

    for (int i = 1; i < argc; i++)
        if (argv[i][0] == '-')
            for (size_t j = 0; j < strlen(argv[i]); j++)
                argv[i][j] = static_cast<char>(toupper(argv[i][j]));

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "-HELP") == 0)
        {
            args->help = TRUE;
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
                printf_s("\nExpected the port number after '-PORT'\n");
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
                printf_s("\nExpected the file path after '-FILE'\n");
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
                printf_s("\nExpected the process name after '-PROCNAME'\n");
                printf_s("type -h or -help for help\n");
                return FALSE;
            }
            args->procname = (PSTR)malloc((strlen(argv[i]) * sizeof(char)) + 1);
            memcpy_s((void*)args->procname, (strlen(argv[i]) * sizeof(char)) + 1, argv[i], strlen(argv[i]) + 1);
        }
        else
        {
            printf_s("\n'%s' was unexpected\n", argv[i]);
            printf_s("type -h or -help for help\n");
            return FALSE;
        }
    }

    if (!(args->open ^ args->attach))
    {
        printf_s("\nYou need to specify the type 'open' or 'attach'\n");
        printf_s("type -h or -help for help\n");
        return FALSE;
    }

    return TRUE;
}

void FreeRunArgs(run_args* pArgs)
{
    free((void*)pArgs->file);
    free((void*)pArgs->port);
    free((void*)pArgs->procname);
}
