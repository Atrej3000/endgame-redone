// Header self-containment check -- confirms inc/ai_forces.h compiles
// standalone in an otherwise empty translation unit. See
// docs/ai-forces-separation-map.md.
#include "ai_forces.h"

int main(void)
{
    return 0;
}
