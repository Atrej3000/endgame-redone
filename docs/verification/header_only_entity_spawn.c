// Header self-containment check -- confirms inc/entity_spawn.h compiles
// standalone in an otherwise empty translation unit. See
// docs/gamestate-decomposition.md section 6.
#include "entity_spawn.h"

int main(void)
{
    return 0;
}
