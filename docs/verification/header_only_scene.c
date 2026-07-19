// Header self-containment check -- confirms inc/scene.h compiles standalone
// in an otherwise empty translation unit. See
// docs/gamestate-decomposition.md section 6.
#include "scene.h"

int main(void)
{
    return 0;
}
