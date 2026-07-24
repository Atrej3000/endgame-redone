#include "header.h"

static char *copy_path(const char *path)
{
    const size_t pathLength = strlen(path);
    if (pathLength == SIZE_MAX) return NULL;
    const size_t length = pathLength + 1U;
    char *copy = (char *)malloc(length);
    if (copy != NULL)
    {
        memcpy(copy, path, length);
    }
    return copy;
}

static bool path_is_absolute(const char *path)
{
    return path[0] == '/' || path[0] == '\\' ||
           (path[0] != '\0' && path[1] == ':' &&
            (path[2] == '/' || path[2] == '\\'));
}

static const char *without_current_directory_prefix(const char *path)
{
    while (path[0] == '.' && (path[1] == '/' || path[1] == '\\'))
    {
        path += 2;
    }
    return path;
}

static char *join_path(const char *directory, const char *relativePath)
{
    const size_t directoryLength = strlen(directory);
    const size_t relativeLength = strlen(relativePath);
    const bool needsSeparator = directoryLength > 0U &&
                                directory[directoryLength - 1U] != '/' &&
                                directory[directoryLength - 1U] != '\\';
    const size_t separatorLength = needsSeparator ? 1U : 0U;
    if (directoryLength > SIZE_MAX - separatorLength ||
        directoryLength + separatorLength > SIZE_MAX - relativeLength ||
        directoryLength + separatorLength + relativeLength == SIZE_MAX)
    {
        return NULL;
    }
    const size_t resultLength = directoryLength + separatorLength +
                                relativeLength + 1U;
    char *result = (char *)malloc(resultLength);
    if (result == NULL)
    {
        return NULL;
    }

    (void)snprintf(result, resultLength, "%s%s%s", directory,
                   needsSeparator ? "/" : "", relativePath);
    return result;
}

static bool path_is_readable(const char *path)
{
    SDL_RWops *stream = SDL_RWFromFile(path, "rb");
    if (stream == NULL)
    {
        return false;
    }
    (void)SDL_RWclose(stream);
    return true;
}

static char *parent_directory(const char *directory)
{
    char *parent = copy_path(directory);
    if (parent == NULL)
    {
        return NULL;
    }

    size_t length = strlen(parent);
    while (length > 0U && (parent[length - 1U] == '/' || parent[length - 1U] == '\\'))
    {
        parent[--length] = '\0';
    }
    while (length > 0U && parent[length - 1U] != '/' && parent[length - 1U] != '\\')
    {
        length--;
    }
    if (length == 0U)
    {
        free(parent);
        return NULL;
    }
    parent[length] = '\0';
    return parent;
}

// Runtime assets are resolved relative to the executable, not the caller's
// current working directory. Development binaries live in build-mingw/, so a
// one-level parent fallback keeps those binaries able to use the repository's
// resource/ tree. The original path remains a final compatibility fallback.
static char *resolve_asset_path(const char *path)
{
    if (path_is_absolute(path))
    {
        return copy_path(path);
    }

    const char *relativePath = without_current_directory_prefix(path);
    char *basePath = SDL_GetBasePath();
    if (basePath != NULL)
    {
        char *candidate = join_path(basePath, relativePath);
        if (candidate != NULL && path_is_readable(candidate))
        {
            SDL_free(basePath);
            return candidate;
        }
        free(candidate);

        char *parentPath = parent_directory(basePath);
        if (parentPath != NULL)
        {
            candidate = join_path(parentPath, relativePath);
            free(parentPath);
            if (candidate != NULL && path_is_readable(candidate))
            {
                SDL_free(basePath);
                return candidate;
            }
            free(candidate);
        }
        SDL_free(basePath);
    }

    return copy_path(path);
}

bool load_texture(SDL_Renderer *renderer, const char *path, SDL_Texture **out_texture)
{
    if (!renderer || !path || !out_texture)
    {
        fprintf(stderr, "load_texture: invalid arguments\n");
        return false;
    }
    if (*out_texture != NULL)
    {
        fprintf(stderr, "load_texture: refusing to overwrite an already-loaded texture (%s)\n", path);
        return false;
    }

    *out_texture = NULL;

    char *resolvedPath = resolve_asset_path(path);
    if (resolvedPath == NULL)
    {
        fprintf(stderr, "load_texture: could not allocate a resolved path for '%s'\n", path);
        return false;
    }

    SDL_Surface *surface = IMG_Load(resolvedPath);
    if (!surface)
    {
        fprintf(stderr, "load_texture: failed to load '%s' (resolved as '%s'): %s\n",
                path, resolvedPath, IMG_GetError());
        free(resolvedPath);
        return false;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture)
    {
        fprintf(stderr, "load_texture: failed to create texture from '%s' (resolved as '%s'): %s\n",
                path, resolvedPath, SDL_GetError());
        free(resolvedPath);
        return false;
    }

    free(resolvedPath);
    *out_texture = texture;
    return true;
}

bool load_music(const char *path, Mix_Music **out_music)
{
    if (!path || !out_music)
    {
        fprintf(stderr, "load_music: invalid arguments\n");
        return false;
    }
    if (*out_music != NULL)
    {
        fprintf(stderr, "load_music: refusing to overwrite already-loaded music (%s)\n", path);
        return false;
    }

    *out_music = NULL;

    char *resolvedPath = resolve_asset_path(path);
    if (resolvedPath == NULL)
    {
        fprintf(stderr, "load_music: could not allocate a resolved path for '%s'\n", path);
        return false;
    }

    Mix_Music *music = Mix_LoadMUS(resolvedPath);
    if (!music)
    {
        fprintf(stderr, "load_music: failed to load '%s' (resolved as '%s'): %s\n",
                path, resolvedPath, Mix_GetError());
        free(resolvedPath);
        return false;
    }

    free(resolvedPath);
    *out_music = music;
    return true;
}

bool load_chunk(const char *path, Mix_Chunk **out_chunk)
{
    if (!path || !out_chunk)
    {
        fprintf(stderr, "load_chunk: invalid arguments\n");
        return false;
    }
    if (*out_chunk != NULL)
    {
        fprintf(stderr, "load_chunk: refusing to overwrite already-loaded chunk (%s)\n", path);
        return false;
    }

    *out_chunk = NULL;
    char *resolvedPath = resolve_asset_path(path);
    if (resolvedPath == NULL)
    {
        fprintf(stderr, "load_chunk: could not allocate a resolved path for '%s'\n", path);
        return false;
    }

    *out_chunk = Mix_LoadWAV(resolvedPath);
    if (!*out_chunk)
    {
        fprintf(stderr, "load_chunk: failed to load '%s' (resolved as '%s'): %s\n",
                path, resolvedPath, Mix_GetError());
        free(resolvedPath);
        return false;
    }
    free(resolvedPath);
    return true;
}

bool load_font(const char *path, int ptsize, TTF_Font **out_font)
{
    if (!path || !out_font)
    {
        fprintf(stderr, "load_font: invalid arguments\n");
        return false;
    }
    if (*out_font != NULL)
    {
        fprintf(stderr, "load_font: refusing to overwrite an already-loaded font (%s)\n", path);
        return false;
    }

    *out_font = NULL;

    char *resolvedPath = resolve_asset_path(path);
    if (resolvedPath == NULL)
    {
        fprintf(stderr, "load_font: could not allocate a resolved path for '%s'\n", path);
        return false;
    }

    TTF_Font *font = TTF_OpenFont(resolvedPath, ptsize);
    if (!font)
    {
        fprintf(stderr, "load_font: failed to load '%s' (resolved as '%s') at size %d: %s\n",
                path, resolvedPath, ptsize, TTF_GetError());
        free(resolvedPath);
        return false;
    }

    free(resolvedPath);
    *out_font = font;
    return true;
}
