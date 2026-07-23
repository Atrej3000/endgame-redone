#include "leaderboard.h"
#include "atomic_file.h"
#include "preferences.h"

#include <errno.h>
#include <inttypes.h>

#define LEADERBOARD_FILE "leaderboard.ini"
#define LEADERBOARD_VERSION 1

static int compare_scores_descending(const void *left, const void *right)
{
    const int leftScore = *(const int *)left;
    const int rightScore = *(const int *)right;
    return (leftScore < rightScore) - (leftScore > rightScore);
}

static bool parse_int_text(const char *text, int *value)
{
    if (!text || !value || text[0] == '\0') return false;
    errno = 0;
    char *end = NULL;
    const intmax_t parsed = strtoimax(text, &end, 10);
    if (errno == ERANGE || end == text || !end || end[0] != '\0' ||
        parsed < (intmax_t)INT_MIN || parsed > (intmax_t)INT_MAX)
    {
        return false;
    }
    *value = (int)parsed;
    return true;
}

static bool parse_line(char *line, char *key, size_t keyCapacity, int *value)
{
    if (!line || !key || keyCapacity == 0U || !value) return false;
    char *separator = strchr(line, '=');
    if (!separator || separator == line || separator[1] == '\0')
    {
        return false;
    }
    const size_t keyLength = (size_t)(separator - line);
    if (keyLength >= keyCapacity) return false;
    memcpy(key, line, keyLength);
    key[keyLength] = '\0';
    return parse_int_text(separator + 1, value);
}

void leaderboard_reset(GameState *game)
{
    if (!game) return;
    for (int i = 0; i < LEADERBOARD_CAPACITY; i++)
    {
        game->x_list[i] = 0;
    }
    game->x_i = 0;
}

size_t leaderboard_copy_top_scores(const GameState *game, int *scores,
                                   size_t capacity)
{
    if (!game || !scores || capacity == 0U) return 0U;

    int count = game->x_i;
    if (count < 0) count = 0;
    if (count > LEADERBOARD_CAPACITY) count = LEADERBOARD_CAPACITY;
    size_t copied = (size_t)count;
    if (copied > capacity) copied = capacity;
    for (size_t i = 0U; i < copied; i++)
    {
        scores[i] = game->x_list[i] < 0 ? 0 : game->x_list[i];
    }
    qsort(scores, copied, sizeof(scores[0]), compare_scores_descending);
    return copied;
}

bool leaderboard_save(const GameState *game)
{
    if (!game) return false;

    int scores[LEADERBOARD_CAPACITY] = {0};
    const size_t count = leaderboard_copy_top_scores(
        game, scores, LEADERBOARD_CAPACITY);
    char contents[1024] = "";
    size_t offset = 0U;
    int written = snprintf(contents, sizeof(contents),
                           "version=%d\ncount=%zu\n",
                           LEADERBOARD_VERSION, count);
    if (written < 0 || (size_t)written >= sizeof(contents)) return false;
    offset = (size_t)written;
    for (size_t i = 0U; i < count; i++)
    {
        written = snprintf(contents + offset, sizeof(contents) - offset,
                           "score%zu=%d\n", i, scores[i]);
        if (written < 0 || (size_t)written >= sizeof(contents) - offset)
        {
            return false;
        }
        offset += (size_t)written;
    }

    char *path = preferences_file_path(LEADERBOARD_FILE);
    if (!path) return false;
    const bool saved = atomic_write_text_file(path, contents, offset);
    free(path);
    return saved;
}

void leaderboard_record_score(GameState *game, int score)
{
    if (!game || score < 0) return;

    int scores[LEADERBOARD_CAPACITY] = {0};
    size_t count = leaderboard_copy_top_scores(
        game, scores, LEADERBOARD_CAPACITY);
    if (count < LEADERBOARD_CAPACITY)
    {
        scores[count] = score;
        count++;
    }
    else if (score > scores[count - 1U])
    {
        scores[count - 1U] = score;
    }
    else
    {
        return;
    }
    qsort(scores, count, sizeof(scores[0]), compare_scores_descending);
    for (size_t i = 0U; i < count; i++)
    {
        game->x_list[i] = scores[i];
    }
    for (size_t i = count; i < LEADERBOARD_CAPACITY; i++)
    {
        game->x_list[i] = 0;
    }
    game->x_i = (int)count;

    if (!leaderboard_save(game))
    {
        fprintf(stderr, "leaderboard_record_score: could not save leaderboard\n");
    }
}

void leaderboard_load(GameState *game)
{
    if (!game) return;
    leaderboard_reset(game);

    size_t contentLength = 0U;
    char *content = preferences_load_bounded_text(
        LEADERBOARD_FILE, 4096U, &contentLength);
    if (!content) return;

    int version = -1;
    int count = -1;
    int scores[LEADERBOARD_CAPACITY] = {0};
    bool scoreSeen[LEADERBOARD_CAPACITY] = {false};
    bool versionSeen = false;
    bool countSeen = false;
    bool valid = true;
    char *cursor = content;
    const char *end = content + contentLength;
    while (cursor < end && valid)
    {
        char *lineEnd = memchr(cursor, '\n', (size_t)(end - cursor));
        const size_t lineLength =
            lineEnd ? (size_t)(lineEnd - cursor) : (size_t)(end - cursor);
        if (memchr(cursor, '\0', lineLength) != NULL)
        {
            valid = false;
            break;
        }
        if (lineEnd) *lineEnd = '\0';
        char *carriageReturn = strchr(cursor, '\r');
        if (carriageReturn)
        {
            if (carriageReturn[1] != '\0')
            {
                valid = false;
                break;
            }
            *carriageReturn = '\0';
        }

        char key[32] = "";
        int value = 0;
        if (!parse_line(cursor, key, sizeof(key), &value))
        {
            valid = false;
        }
        else if (strcmp(key, "version") == 0)
        {
            if (versionSeen) valid = false;
            versionSeen = true;
            version = value;
        }
        else if (strcmp(key, "count") == 0)
        {
            if (countSeen) valid = false;
            countSeen = true;
            count = value;
        }
        else
        {
            int index = -1;
            if (strncmp(key, "score", 5U) != 0 ||
                !parse_int_text(key + 5, &index) ||
                index < 0 || index >= LEADERBOARD_CAPACITY ||
                scoreSeen[index] || value < 0)
            {
                valid = false;
            }
            else
            {
                scoreSeen[index] = true;
                scores[index] = value;
            }
        }
        cursor = lineEnd ? lineEnd + 1 : (char *)end;
    }
    SDL_free(content);

    if (!valid || !versionSeen || version != LEADERBOARD_VERSION ||
        !countSeen || count < 0 || count > LEADERBOARD_CAPACITY)
    {
        return;
    }
    for (int i = 0; i < count; i++)
    {
        if (!scoreSeen[i]) return;
    }
    for (int i = count; i < LEADERBOARD_CAPACITY; i++)
    {
        if (scoreSeen[i]) return;
    }

    qsort(scores, (size_t)count, sizeof(scores[0]), compare_scores_descending);
    for (int i = 0; i < count; i++)
    {
        game->x_list[i] = scores[i];
    }
    game->x_i = count;
}

static void draw_leaderboard_text(GameState *game, const char *text,
                                  int x, int y)
{
    SDL_Color color = {38, 24, 56, 255};
    SDL_Surface *surface = TTF_RenderText_Blended(game->font, text, color);
    if (!surface) return;
    SDL_Texture *texture =
        SDL_CreateTextureFromSurface(game->app.renderer, surface);
    if (texture)
    {
        SDL_Rect destination = {x, y, surface->w, surface->h};
        (void)SDL_RenderCopy(game->app.renderer, texture, NULL, &destination);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

void init_status_x_list(GameState *game)
{
    if (!game || !game->font || !game->app.renderer) return;
    int scores[LEADERBOARD_CAPACITY] = {0};
    const size_t count = leaderboard_copy_top_scores(
        game, scores, LEADERBOARD_CAPACITY);
    if (count == 0U)
    {
        draw_leaderboard_text(game, "No scores yet", 535, 150);
        return;
    }

    // Thirteen rows per column keep every one of the 25 stored scores inside
    // the 1280x720 logical canvas.
    for (size_t i = 0U; i < count; i++)
    {
        char text[40] = "";
        (void)snprintf(text, sizeof(text), "%02zu. %d points",
                       i + 1U, scores[i]);
        const int column = i < 13U ? 0 : 1;
        const int row = column == 0 ? (int)i : (int)(i - 13U);
        draw_leaderboard_text(game, text,
                              column == 0 ? 210 : 730,
                              115 + row * 40);
    }
}
