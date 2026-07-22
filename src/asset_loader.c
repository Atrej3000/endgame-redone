#include "header.h"

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

    SDL_Surface *surface = IMG_Load(path);
    if (!surface)
    {
        fprintf(stderr, "load_texture: failed to load '%s': %s\n", path, IMG_GetError());
        return false;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture)
    {
        fprintf(stderr, "load_texture: failed to create texture from '%s': %s\n", path, SDL_GetError());
        return false;
    }

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

    Mix_Music *music = Mix_LoadMUS(path);
    if (!music)
    {
        fprintf(stderr, "load_music: failed to load '%s': %s\n", path, Mix_GetError());
        return false;
    }

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
    *out_chunk = Mix_LoadWAV(path);
    if (!*out_chunk)
    {
        fprintf(stderr, "load_chunk: failed to load '%s': %s\n", path, Mix_GetError());
        return false;
    }
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

    TTF_Font *font = TTF_OpenFont(path, ptsize);
    if (!font)
    {
        fprintf(stderr, "load_font: failed to load '%s' at size %d: %s\n", path, ptsize, TTF_GetError());
        return false;
    }

    *out_font = font;
    return true;
}
