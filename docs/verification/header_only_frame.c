// Header self-containment check -- confirms inc/frame.h compiles standalone
// in an otherwise empty translation unit. See
// docs/gamestate-decomposition.md section 6.
#include "frame.h"

int main(void)
{
    return 0;
}
