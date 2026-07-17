#include "header.h"

char *mx_itoa(int number) {
    long int buf = number;
    long int buf2 = number;
    int len = 1;
    char *new_str = NULL;

    if (number) {
        buf < 0 ? (len++, buf *= -1) : len;
        for (; number / 10; len++)
            number /= 10;
        new_str = mx_strnew(len);
        for (len -= 1; len >= 0; buf /= 10)
            new_str[len--] = (buf % 10) + '0';
        buf2 < 0 ? (new_str[0] = '-') : new_str[0];
    }
    if(number == 0)
        new_str = "0";
    return new_str;
}
