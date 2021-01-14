#include <stdio.h>
#include <stdlib.h>
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
