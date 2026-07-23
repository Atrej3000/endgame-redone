#pragma once

#include "header.h"

// Caller owns the returned UTF-8 path with free(). ENDGAME_PREF_DIR is a
// deliberate test/support override; normal players use SDL_GetPrefPath.
// filename must be a non-empty leaf name (no separators or traversal).
char *preferences_file_path(const char *filename);

// Reads at most maxLength bytes after checking the file size through SDL's
// RWops API, before allocating. The returned NUL-terminated buffer is owned
// by the caller and must be released with SDL_free(). Oversized, non-regular,
// short-read, and invalid files return NULL with *lengthOut reset to zero.
char *preferences_load_bounded_text(const char *filename, size_t maxLength,
                                    size_t *lengthOut);
