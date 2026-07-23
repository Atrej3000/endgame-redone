#include "preferences.h"

#include <errno.h>
#include <stdint.h>

static bool valid_leaf_filename(const char *filename)
{
    return filename != NULL && filename[0] != '\0' &&
           strcmp(filename, ".") != 0 && strcmp(filename, "..") != 0 &&
           strchr(filename, '/') == NULL && strchr(filename, '\\') == NULL;
}

char *preferences_file_path(const char *filename)
{
    if (!valid_leaf_filename(filename)) return NULL;

    const char *overrideDirectory = getenv("ENDGAME_PREF_DIR");
    char *sdlDirectory = NULL;
    const char *directory = overrideDirectory;
    if (!directory || directory[0] == '\0')
    {
        sdlDirectory = SDL_GetPrefPath("Ucode", "Endgame");
        directory = sdlDirectory;
    }
    if (!directory) return NULL;

    const size_t directoryLength = strlen(directory);
    const bool needsSeparator = directoryLength > 0U &&
                                directory[directoryLength - 1U] != '/' &&
                                directory[directoryLength - 1U] != '\\';
    const size_t separatorLength = needsSeparator ? 1U : 0U;
    const size_t filenameLength = strlen(filename);
    if (directoryLength > SIZE_MAX - separatorLength ||
        directoryLength + separatorLength > SIZE_MAX - filenameLength - 1U)
    {
        if (sdlDirectory) SDL_free(sdlDirectory);
        errno = ENAMETOOLONG;
        return NULL;
    }
    const size_t length = directoryLength + separatorLength + filenameLength + 1U;
    char *path = (char *)malloc(length);
    if (path)
    {
        (void)snprintf(path, length, "%s%s%s", directory,
                       needsSeparator ? "/" : "", filename);
    }
    if (sdlDirectory) SDL_free(sdlDirectory);
    return path;
}

char *preferences_load_bounded_text(const char *filename, size_t maxLength,
                                    size_t *lengthOut)
{
    if (lengthOut) *lengthOut = 0U;
    if (!lengthOut) return NULL;

    char *path = preferences_file_path(filename);
    if (!path) return NULL;
    SDL_RWops *stream = SDL_RWFromFile(path, "rb");
    free(path);
    if (!stream) return NULL;

    const Sint64 signedLength = SDL_RWsize(stream);
    if (signedLength < 0 ||
        (Uint64)signedLength > (Uint64)maxLength ||
        (Uint64)signedLength > (Uint64)(SIZE_MAX - 1U))
    {
        (void)SDL_RWclose(stream);
        return NULL;
    }

    const size_t length = (size_t)signedLength;
    char *content = (char *)SDL_malloc(length + 1U);
    if (!content)
    {
        (void)SDL_RWclose(stream);
        return NULL;
    }
    if (length > 0U && SDL_RWread(stream, content, 1U, length) != length)
    {
        SDL_free(content);
        (void)SDL_RWclose(stream);
        return NULL;
    }
    (void)SDL_RWclose(stream);
    content[length] = '\0';
    *lengthOut = length;
    return content;
}
