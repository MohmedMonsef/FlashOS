#include <stdio.h>
#include <stdlib.h>

FILE *fMemoryLog;

int ceil_int(double x)
{
    return (int)x + 1;
}
int log2_int(int x)
{
    int power = 0;
    int res = 1;
    while (res < x)
    {
        res = res * 2;
        power += 1;
    }
    return power;
}
int pow2(int x)
{

    int res = 1;
    while (x--)
    {
        res = res * 2;
    }
    return res;
}
void openMemoryLogFile()
{
    fMemoryLog = fopen("memory.log", "w");
}
void closeMemoryLogFile()
{
    fclose(fMemoryLog);
}
void writeInMemoryFile(char **params, int size)
{
    char *firstStrings[6] = {"At time  ", "   ", " ", " Bytes For Process ", " from ", "  to  "};
    char *strOut = (char *)malloc(500);
    for (int i = 0; i < size; i++)
    {
        strcat(strOut, firstStrings[i]);
        strcat(strOut, params[i]);
    }
    fprintf(fMemoryLog, "%s\n", strOut);
}

void logProcessMemory(int id, int processSize, int startOffset, int endOffset, char *status, int clk)
{
    int size = 6;
    char *params[6];
    id += 1;
    for (int i = 0; i < 6; i++)
    {
        params[i] = (char *)malloc(50 * sizeof(char));
    }
    sprintf(params[0], "%d", clk);
    params[1] = status;
    sprintf(params[2], "%d", processSize);
    sprintf(params[3], "%d", id);
    sprintf(params[4], "%d", startOffset);
    sprintf(params[5], "%d", endOffset);
    writeInMemoryFile(params, size);
}