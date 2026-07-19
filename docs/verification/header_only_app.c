// Header self-containment check -- confirms inc/app.h compiles standalone
// in an otherwise empty translation unit (no reliance on some other header
// having been included first). See docs/gamestate-decomposition.md section 6.
#include "app.h"

int main(void)
{
    return 0;
}
