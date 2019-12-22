#include <iostream>
#include <Windows.h>


int main()
{
	while (1)
	{
		int c = EOF;
		while ((c = getc(stdin)) != EOF)
		{
			std::cout << (char)c;
		}
		Sleep(2000);
	}
}