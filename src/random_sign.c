#include "header.h"
#include <stdint.h>

int random_sign(int mult, int step)
{
    const int sign = (random() % 2 == 0) ? 1 : -1;
    const int64_t magnitude = (int64_t)step * (int64_t)mult;
    const int64_t value = magnitude * sign;
    if (value > INT_MAX) return INT_MAX;
    if (value < INT_MIN) return INT_MIN;
    return (int)value;
}
