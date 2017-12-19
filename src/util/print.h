#ifndef PRINT_H
#define PRINT_H

int print_uchar(unsigned char v);

int print_char(char v);

int print_short(short v);

int print_ushort(unsigned short v);

int print_int(int v);

int print_uint(unsigned v);

int print_long(long v);

int print_ulong(unsigned long v);

int print_long_long(long long v);

int print_ulong_long(unsigned long long v);

int print_float(float v);

int print_double(double v);

int print_long_double(long double v);

int print_string(char *s);

int print_pointer(void *v);

int print_unknown();

#define print(x) \
    _Generic((x), \
    unsigned char: print_uchar, \
    char: print_char, \
    short int: print_short, \
    unsigned short int: print_ushort, \
    int: print_int, \
    unsigned int: print_uint, \
    long int: print_long, \
    unsigned long int: print_ulong, \
    long long int: print_long_long, \
    unsigned long long int: print_ulong_long, \
    float: print_float,          \
    double: print_double,                 \
    long double: print_long_double,     \
    char *: print_string, \
    void *: print_pointer, \
    default : print_unknown)(x)

#endif
