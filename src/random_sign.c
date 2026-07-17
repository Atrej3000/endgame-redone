#include "header.h"
#include <math.h>

int random_sign(int mult, int step) {
    step = step * mult * pow(-1, (random()%2));
    return step;
}
