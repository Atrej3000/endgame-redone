#pragma once

// On Windows, SDL2's headers rewrite main() into a signature expecting
// (int argc, char *argv[]) so SDL2main can hook WinMain. This project's
// main() takes no arguments; SDL_MAIN_HANDLED opts out of that rewrite so
// the existing signature keeps compiling as-is. No effect on macOS/Linux.
#ifdef _WIN32
#define SDL_MAIN_HANDLED
#endif

//our frameworks
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <SDL2_ttf/SDL_ttf.h>
#include <SDL2_mixer/SDL_mixer.h>

//system libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#ifdef __APPLE__
#include <malloc/malloc.h>
#endif
#include <limits.h>

// random()/srandom() are POSIX/BSD extensions with no equivalent in the
// Windows CRT that MinGW targets; rand()/srand() are the nearest portable
// stand-ins. Only takes effect on Windows builds -- macOS/Linux keep using
// the real random()/srandom() unchanged. Routed through a differently-named
// wrapper (not a bare "rand()" expansion) because at least one call site
// declares a local variable literally named "rand", which would otherwise
// shadow the C library rand() the macro expands to.
#ifdef _WIN32
static inline long ucode_endgame_win32_random(void) { return rand(); }
#define random() ucode_endgame_win32_random()
#define srandom(seed) srand(seed)
#endif

#define STATUS_STATE_LIVES 0
#define STATUS_STATE_GAME 1
#define WIDTH 1280
#define HEIGHT 720
#define GRAVITY 0.5f
#define MAX_BULLETS 1000

#define NUM_STARS 100
#define NUM_ENEMIES 101
#define NUM_SMART_ENEMIES 10

typedef struct 
{
    float x, y, w, h;
    float dx, dy;
    int slowingDown, onLedge, isdead, visible, countShots;

    int currentSpriteRun, currentSpriteRun2;
    SDL_Texture *sheetTextureRun, *sheetTextureRun2;
} Enemies;

typedef struct 
{
    float x, y, dx;
    int unvisible;
} Bullet;


typedef struct
{
    float x, y, w, h;
    float dx, dy;
    short lives;
    char *name;
    int onLedge, isDead;//, isdead, visible, countShots;

    int animFrame, animFrameSecond, currentSpriteIdle, currentSpriteRun, currentSpriteRun2,
        currentSpriteJump, currentSpriteJump2,
        facingLeft, slowingDown, visible0;
    SDL_Texture *sheetTextureIdle, *sheetTextureRun,
            *sheetTextureRun2, *sheetTextureJump, *sheetTextureJump2;
} Man;

typedef struct
{
    int x, y, baseX, baseY, mode;
    float dx, dy, phase;
} Star;

typedef struct
{
    int x, y, w, h;
} Ledge;

typedef struct
{
    int x, y;
    SDL_Texture *textureTrain;
} Train;

typedef struct
{
    int x, y;

    SDL_Texture *sheetTextureCloud1;
}Cloud1;

typedef struct
{
    int x, y;

    SDL_Texture *sheetTextureCloud2;
}Cloud2;

typedef struct
{
    int x, y;

    SDL_Texture *sheetTextureCloud3;
}Cloud3;

typedef struct
{
    int x, y;

    SDL_Texture *sheetTextureCloud4;
}Cloud4;

typedef struct
{
    int x, y;

    SDL_Texture *sheetTextureCloud5;
}Cloud5;

typedef struct
{
    int x, y;

    SDL_Texture *sheetTextureCloud6;
}Cloud6;

typedef struct
{
    int x, y;

    SDL_Texture *sheetTextureCloud7;
}Cloud7;

typedef struct
{
    int x, y;

    SDL_Texture *sheetTextureCloud8;
}Cloud8;

// Top-level application scene. The single authoritative field driving all
// screen routing -- see docs/scene-state-map.md for the full transition
// table. game->scene is written ONLY inside app_change_scene() (src/scene.c);
// every other file that changes scenes must call app_change_scene(), never
// assign this field directly.
typedef enum AppScene
{
    APP_SCENE_MAIN_MENU,
    APP_SCENE_ARCADE_MENU,
    APP_SCENE_ARCADE_GAME,
    APP_SCENE_ARCADE_LEADERBOARD,
    APP_SCENE_ARCADE_PAUSE,
    APP_SCENE_RUNNER_MENU,
    APP_SCENE_RUNNER_GAME,
    APP_SCENE_RUNNER_LEADERBOARD,
    APP_SCENE_RUNNER_PAUSE,
    APP_SCENE_QUIT
} AppScene;

// Single/multiplayer selection. Introduced here (ahead of GameState.multiPlayer's
// eventual retype from int to GameMode) because arcade_session_reset()/
// runner_session_reset() (src/loadGame.c) take it as a parameter -- see
// docs/game-session-lifecycle.md.
typedef enum GameMode
{
    GAME_MODE_SINGLE_PLAYER,
    GAME_MODE_MULTIPLAYER
} GameMode;

typedef struct
{
    // scroll thw world
    float scrollX;
    // Single/multiplayer selection. Written only by arcade_session_reset()/
    // runner_session_reset() (src/loadGame.c) as of the mode-lifecycle
    // refactor -- previously also hardcoded by the old loadGame() on every
    // call, which is the bug that refactor fixed. Field name kept as
    // "multiPlayer" (not renamed to "mode") to avoid touching its ~26
    // existing boolean-truth-test read sites across process.c/
    // processEvents.c/doRender.c/collisionDetect.c/kills_score.c -- their
    // behavior is unaffected, since GameMode's values (0/1) match the int's
    // previous values exactly.
    GameMode multiPlayer;

    //Players
    int gameLives;

    int sumScore;
    int tempScore;
    int shotCount;
    int shotCountMultiplayer;

    Man secondPlayer;
    Man man;
    Enemies enemy;
    Enemies boss[2];
    Enemies enemyValues[NUM_ENEMIES];
    Enemies smartEnemies[NUM_SMART_ENEMIES];
    Train train;
    Cloud1 cloud1;
    Cloud2 cloud2;
    Cloud3 cloud3;
    Cloud4 cloud4;
    Cloud5 cloud5;
    Cloud6 cloud6;
    Cloud7 cloud7;
    Cloud8 cloud8;

    Bullet *bullets[MAX_BULLETS];
    Bullet *secondBullets[MAX_BULLETS];

    //Stars
    Star stars[100];

    //Ledges
    Ledge ledges[100];

    //Bullet
    int CurrentSheetBullet;
    int CurrentSheetBullet2;

    //Boss
    int CurrentSheetBoss;

    //Images
    SDL_Texture *star;
    SDL_Texture *manFrames[12];
    SDL_Texture *secondPlayerFrames[12];
    SDL_Texture *brick;
    SDL_Texture *menu0;
    SDL_Texture *menu1;
    SDL_Texture *menu2;
    SDL_Texture *mult;
    SDL_Texture *leaders;
    SDL_Texture *fon;
    SDL_Texture *pause;
    SDL_Texture *label;
    SDL_Texture *labelMultiplayer;
    SDL_Texture *death;
    SDL_Texture *brick_block;
    SDL_Texture *copper_block;
    SDL_Texture *bulletTexture;
    SDL_Texture *secondBulletTexture;
    SDL_Texture *bossTexture;
    int labelW, labelH;

    //Background
    int CurrentSpriteBack, CurrentSpriteBack2, CurrentSpriteBack3;
    SDL_Texture *sheetTextureBack, *sheetTextureBack2, *sheetTextureSun;

    // Fonts
    TTF_Font *font;

    //SOUNDS
    Mix_Music *menuMus, *battleMus, *runnerMus;
    Mix_Chunk *jumpSound, *kickSound, *select, *shootSound, *damageSound;

    //Renderer
    SDL_Renderer *renderer;

    // Explicit asset-group lifecycle flags. Each is set to true only after
    // its entire group's loads succeed (see arcade_assets_load/
    // runner_assets_load/shared_assets_load, src/loadGame.c), and reset to
    // false by the matching *_assets_unload(). sharedAssetsLoaded covers the
    // handful of textures/font both modes load identically (mult, leaders,
    // pause, brick, death, font) -- see docs/game-session-lifecycle.md.
    bool arcadeAssetsLoaded;
    bool runnerAssetsLoaded;
    bool sharedAssetsLoaded;

    // Authoritative scene -- write ONLY via app_change_scene() (src/scene.c).
    AppScene scene;

    // DEPRECATED: superseded by `scene` (AppScene) above. No longer read or
    // written anywhere in active routing code as of the scene-state refactor;
    // kept declared (not deleted) per that phase's own scope rules. Safe to
    // remove entirely in a future phase.
    int menu_status;
    int menu0_status;
    int x_score;
    int x_list[25];
    //char *x_names[20];
    int x_i;
    int kills_score;
    int kills_score_multi;
    int time, deathCountdown;
    int statusState;

    int iter;

    //SDL_Event event;

} GameState;

    //something for leaderboard
//    int leaderboard_list[25] = {0};
//    int *leader_list = leaderboard_list;clear

// PRototypes

// Mode asset/session lifecycle (src/loadGame.c) -- see
// docs/game-session-lifecycle.md. Replaces the former loadGame()/loadGame2(),
// which combined asset loading and session reset in one function.
// (shared_assets_load() is internal to loadGame.c -- shared_assets_unload()
// is exposed because app_shutdown() calls it.)
void shared_assets_unload(GameState *game);
bool arcade_assets_load(GameState *game);
void arcade_assets_unload(GameState *game);
void arcade_session_reset(GameState *game, GameMode mode);
bool runner_assets_load(GameState *game);
void runner_assets_unload(GameState *game);
void runner_session_reset(GameState *game, GameMode mode);

void process(GameState *game);
void collisionDetect(GameState *game);
int processEvents(SDL_Window *window, GameState *game);
void doRender(SDL_Renderer *renderer, GameState *game);
void shutdown_status_lives (GameState *game);
void draw_status_lives(GameState *game);
void draw_status_lives2(GameState *game);
void init_status_lives(GameState *game);
int  doRender_menu1(SDL_Renderer *renderer, GameState *game);
int  doRender_menu2(SDL_Renderer *renderer, GameState *game);
void doRender_multiplayer(SDL_Renderer *renderer, GameState *game);
int doRender_leaderboard(SDL_Renderer *renderer, GameState *game);
int doRender_pause(SDL_Renderer *renderer, GameState *game);
void doRender(SDL_Renderer *renderer, GameState *game);
int collide2d(float x1, float y1, float x2, float y2, float wt1, float ht1, float wt2, float ht2);
void menu_events(GameState *gameState);
void load_menu1(GameState *game);
void load_menu2(GameState *game);

// Scene routing (src/scene.c) -- see docs/scene-state-map.md
void app_change_scene(GameState *game, AppScene next_scene);

// Per-mode frame pipeline (src/frame.c) -- see docs/frame-pipeline-map.md.
// Each wraps that mode's existing update/collision/render/events calls in
// their unchanged order, giving the pipeline a single named entry point.
void arcade_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer);
void runner_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer);

// Runner death lifecycle (src/runner_death.c) -- see docs/runner-death-lifecycle.md.
void runner_trigger_death(GameState *game);
void runner_update_death(GameState *game);

// Null-safe destroy/free helpers (src/app.c) -- shared by app_shutdown() and
// the mode-specific asset unload helpers (src/loadGame.c)
void destroy_texture(SDL_Texture **tex);
void free_chunk(Mix_Chunk **chunk);
void free_music(Mix_Music **music);

// Checked SDL asset-loading helpers (src/asset_loader.c) -- see
// docs/game-session-lifecycle.md. Each rejects NULL arguments, nulls
// *out before attempting, refuses to silently overwrite an already-loaded
// (non-NULL) destination, frees temporary surfaces on every path, reports
// the failing asset path via stderr, and never calls exit()/SDL_Quit().
bool load_texture(SDL_Renderer *renderer, const char *path, SDL_Texture **out_texture);
bool load_music(const char *path, Mix_Music **out_music);
bool load_font(const char *path, int ptsize, TTF_Font **out_font);
int pause_events(GameState *gameState);
int leader_events(GameState *gameState);
void addBullet(GameState *game, float x, float y, float dx);
void addSecondBullet(GameState *game, float x, float y, float dx);
void removeSecondBullet(GameState *game, int i);

//score
void init_status_kills(GameState *game);
void draw_status_kills(GameState *game);
void init_status_x(GameState *game);
void draw_status_x(GameState *game);
void init_status_x_list(GameState *game);

//for two mods game
void load_menu0(GameState *game);
int  doRender_menu0(SDL_Renderer *renderer, GameState *game);
void menu0_events(GameState *gameState);
void collisionDetect2(GameState *game);
void doRender2(SDL_Renderer *renderer, GameState *game);
void process2(GameState *game);
int processEvents2(SDL_Window *window, GameState *game);
int doRender_leaderboard2(SDL_Renderer *renderer, GameState *game);
void doRender_multiplayer2(SDL_Renderer *renderer, GameState *game);

void removeBullet(GameState *game, int i);

//Leo functions
int random_sign(int mult, int step);
void mx_sort_arr_char(char *arr[], int size);

//mx_lib
char *mx_itoa(int number);
int mx_strlen(const char *s);
//void mx_printerr(const char *s);
char *mx_strcat(char *s1, const char *s2);
char *mx_strcpy(char *dst, const char *src);
char *mx_strdup(const char *str);
char *mx_strjoin(char const *s1, char const *s2);
char *mx_strnew(const int size);
char *mx_file_to_str(const char *filename);
void mx_strdel(char **str);
int mx_count_words(const char *str, char delimiter);
char *mx_strncpy(char *dst, const char *src, int len);
char **mx_strsplit(char const *s, char c);
int mx_strcmp(const char *s1, const char *s2);
void mx_sort_arr_int(int *arr, int size);
int mx_atoi(const char *str);
