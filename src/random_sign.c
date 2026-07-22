#include "header.h"
int random_sign(int mult, int step) {
    const int sign = (random() % 2 == 0) ? 1 : -1;
    return step * mult * sign;
}
