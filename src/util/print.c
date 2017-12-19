#include <stdio.h>

#include "print.h"

int print_uchar(unsigned char v)
{
    return printf("%u", v);
}

int print_char(char v)
{
    return printf("%c", v);
}

int print_short(short v)
{
    return printf("%d", v);
}

int print_ushort(unsigned short v)
{
    return printf("%u", v);
}

int print_int(int v)
{
    return printf("%d", v);
}

int print_uint(unsigned v)
{
    return printf("%u", v);
}

int print_long(long v)
{
    return printf("%ld", v);
}

int print_ulong(unsigned long v)
{
    return printf("%lu", v);
}

int print_long_long(long long v)
{
    return printf("%lld", v);
}

int print_ulong_long(unsigned long long v)
{
    return printf("%llu", v);
}

int print_float(float v)
{
    return printf("%f", (double)v);
}

int print_double(double v)
{
    return printf("%f", v);
}

int print_long_double(long double v)
{
    return printf("%Lf", v);
}

int print_string(char *s)
{
    return printf("%s", s);
}

int print_pointer(void *v)
{
    return printf("%p", v);
}

int print_unknown()
{
    return printf("(variable of unknown type)");
}
