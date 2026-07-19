// Header self-containment check -- confirms inc/input_command.h compiles
// standalone in an otherwise empty translation unit. See
// docs/gamestate-decomposition.md section 6.
#include "input_command.h"

int main(void)
{
    return 0;
}
