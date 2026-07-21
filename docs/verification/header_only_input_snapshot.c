// Header self-containment check -- confirms inc/input_snapshot.h compiles
// standalone in an otherwise empty translation unit. See
// docs/input-snapshot-architecture-map.md.
#include "input_snapshot.h"

int main(void)
{
    return 0;
}
