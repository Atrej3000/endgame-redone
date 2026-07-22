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
#define MAX_BULLETS 1000
#define MAX_GAME_EVENTS 1024

#define NUM_STARS 100
#define NUM_ENEMIES 101
#define NUM_SMART_ENEMIES 10

// Fixed-timestep physics (Phase 11) -- see docs/physics-timestep-map.md for
// the full derivation. process()/process2() are called at this fixed rate
// from main()'s accumulator, decoupled from the render/VSync rate.
#define PHYSICS_HZ 60.0
#define PHYSICS_DT ((float)(1.0 / PHYSICS_HZ))
// Caps a real-time spike (e.g. a debugger pause) so the accumulator can't
// demand hundreds of catch-up ticks in one frame ("spiral of death").
#define MAX_FRAME_TIME 0.25
#define MAX_PHYSICS_STEPS_PER_FRAME 5

// Opt-in performance logging (Phase 16, see docs/optimization-map.md): how
// often, in real seconds, main() prints a physics/render timing summary when
// the ENDGAME_PERF_LOG environment variable is set. No effect otherwise.
#define PERF_LOG_INTERVAL_SEC 1.0

// Player-only physics constants, converted from the previous per-frame
// values to per-second units (velocity: old x60; acceleration: old x3600 --
// see docs/physics-timestep-map.md section 3 for the derivation and the
// regression proof that dt=1/60 reproduces the old per-frame behavior
// exactly). Enemies/bosses/bullets/background/traps are unconverted this
// phase -- still frame-tied, deliberately out of scope.
#define GRAVITY_PER_SEC2 1800.0f              // was GRAVITY 0.5f/frame
#define JUMP_SPEED_PER_SEC 600.0f             // was dy = -10/frame (one-shot impulse)
#define RUN_ACCEL_PER_SEC2 1800.0f            // was dx += 0.5f/frame
#define RUN_MAX_SPEED_PER_SEC 360.0f          // was clamp to +-6/frame
// ARCADE_JUMP_HOLD_ACCEL_PER_SEC2/RUNNER_JUMP_HOLD_ACCEL_PER_SEC2 (the old
// continuous jump-hold-thrust constants) were removed in Phase 15 -- see
// docs/game-feel-map.md. Replaced by JUMP_CUT_SPEED_PER_SEC below.
#define RUN_FRICTION_DECAY_PER_TICK 0.8f      // unchanged multiplicative factor
#define RUN_SNAP_ZERO_SPEED_PER_SEC 6.0f      // was fabsf(dx) < 0.1f/frame

// Player-vs-ledge hitbox, named for the 5 collisionDetect.c blocks Phase 13
// touches (Arcade/Runner man/secondPlayer) -- same 48x48 value every one of
// those blocks already used as a bare literal; see
// docs/collision-correctness-map.md section 5 for why the many *other*
// scattered hitbox literals (enemy/boss/star/etc.) are deliberately left
// untouched rather than reconciled with this one.
#define PLAYER_LEDGE_HITBOX_W 48.0f
#define PLAYER_LEDGE_HITBOX_H 48.0f

// Game feel (Phase 15, see docs/game-feel-map.md). Unlike the fixed-timestep
// conversion constants above, these have no legacy value to preserve -- new
// behavior, reasonable starting defaults, easy to retune since they're named.
#define COYOTE_TICKS 6         // ~100ms at the fixed 60Hz tick
#define JUMP_BUFFER_TICKS 6    // ~100ms
#define JUMP_CUT_SPEED_PER_SEC 200.0f  // roughly a third of JUMP_SPEED_PER_SEC

// Non-player movement uses the same pixels/second and pixels/second-squared
// convention as the players (Phase 21). Each value preserves the old 60 Hz
// behavior: former tick velocities are multiplied by 60; former tick
// accelerations by 3,600.
#define BULLET_SPEED_PER_SEC 678.0f
#define BOSS_GRAVITY_PER_SEC2 360.0f
#define BOSS_MAX_FALL_SPEED_PER_SEC 90.0f
#define BOSS_HORIZONTAL_ACCEL_PER_SEC2 360.0f
#define BOSS_MAX_HORIZONTAL_SPEED_PER_SEC 60.0f
#define ENEMY_GRAVITY_PER_SEC2 1440.0f
#define ENEMY_MAX_FALL_SPEED_PER_SEC 150.0f
#define ENEMY_HORIZONTAL_ACCEL_PER_SEC2 720.0f
#define ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC 120.0f
#define SMART_ENEMY_GRAVITY_PER_SEC2 3600.0f
#define SMART_ENEMY_JUMP_SPEED_PER_SEC 360.0f
#define SMART_ENEMY_HORIZONTAL_ACCEL_PER_SEC2 720.0f
#define SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC 240.0f
#define SMART_ENEMY_HOP_ACCEL_PER_SEC2 3600.0f
#define SMART_ENEMY_HOP_SPEED_PER_SEC 600.0f
#define RUNNER_MULTIPLAYER_CAMERA_SPEED_PER_SEC 180.0f
#define TRAP_ANGULAR_SPEED_PER_SEC 3.6f

typedef struct
{
    float x, y, w, h;
    float dx, dy;
    // Render-only transform from the start of the current fixed tick.
    // Physics and collision detection always use x/y.
    float prevX, prevY;
    int slowingDown, onLedge, isdead, visible, countShots;

    int currentSpriteRun, currentSpriteRun2;
    SDL_Texture *sheetTextureRun, *sheetTextureRun2;
} Enemies;

// Pool slot (Phase 14, see docs/projectile-correctness-map.md): `active`
// replaces the old NULL-pointer-means-empty convention; `prevX` is captured
// once per tick, before this tick's move, for the swept-collision test.
// No `unvisible` field -- traced fully and confirmed redundant with
// `active` (a hit always deactivated the same tick it was detected, never
// a delayed-despawn signal).
typedef struct
{
    float x, y, dx;
    float prevX;
    float prevY;
    bool active;
} Bullet;


typedef struct
{
    float x, y, w, h;
    float dx, dy;
    // Render-only previous horizontal position. prevY remains the
    // authoritative previous-tick position used by landing collision.
    float prevX;
    // y at the end of the previous physics tick, captured by
    // capture_player_prev_y() (src/collisionDetect.c) before this tick's
    // process()/process2() moves y -- lets the ledge landing check
    // distinguish "still resting on a surface" from "just walked off one"
    // (Phase 13, see docs/collision-correctness-map.md).
    float prevY;
    // Coyote time (Phase 15, see docs/game-feel-map.md): refreshed to
    // COYOTE_TICKS every grounded tick, counts down while airborne. A jump
    // still succeeds while this is > 0, even after onLedge has gone false.
    int coyoteTicksRemaining;
    // Last tick's jump-key-held state, for release-edge detection --
    // variable jump height (Phase 15) cuts dy short the tick the key goes
    // from held to not-held while still rising fast.
    bool jumpKeyHeldLastTick;
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
    float prevX, prevY;
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
// table. game->app.scene is written ONLY inside app_change_scene()
// (src/scene.c); every other file that changes scenes must call
// app_change_scene(), never assign this field directly.
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

// Application/platform-level state, as opposed to gameplay state -- see
// docs/gamestate-decomposition.md for the full field-by-field ownership
// audit behind this grouping. `renderer` is owned/set once by app_init(),
// nulled once by app_shutdown() (src/app.c); `scene` is written ONLY via
// app_change_scene() (src/scene.c), with one documented bootstrap exception
// in main.c. `window` deliberately has no field here: it was never a
// GameState field (see inc/app.h's app_init() out-parameters) and adding it
// now would introduce new state rather than migrate existing state.
typedef struct AppContext
{
    SDL_Renderer *renderer;
    AppScene scene;
} AppContext;

// Explicit asset-group lifecycle flags, grouped per
// docs/gamestate-decomposition.md: write surface fully contained to the 3
// loader + 3 unloader functions in src/loadGame.c (see the field-level
// comment retained below).
typedef struct AssetLifecycleFlags
{
    bool arcadeAssetsLoaded;
    bool runnerAssetsLoaded;
    bool sharedAssetsLoaded;
} AssetLifecycleFlags;

// Continuous held-key state (Phase 17, see docs/input-snapshot-architecture-map.md), captured
// exactly once per real frame -- before the fixed-step accumulator loop runs -- by
// input_capture_arcade()/input_capture_runner() (src/input_snapshot.c). apply_arcade_player_forces()/
// apply_runner_player_forces() and processEvents()'s shoot check read these fields instead of
// calling SDL_GetKeyboardState() themselves, so every physics tick and event-poll a given real
// frame produces sees the identical held-key state that frame captured.
//
// Edge-triggered discrete-input request flags, separated from the fixed-tick
// simulation that consumes them -- see docs/input-simulation-separation-map.md
// (Phase 12; the physics assessment's own "Phase 2 -- separate input and
// simulation"). Set by processEvents()/processEvents2() on SDL_KEYDOWN;
// consumed by consume_arcade_jump_requests()/consume_runner_jump_requests()
// (src/process.c) at the fixed physics tick rate.
//
// Retyped from bool to a tick countdown (Phase 15, see docs/game-feel-map.md)
// for jump buffering: nonzero means "a jump is pending, within its buffer
// window" -- set to JUMP_BUFFER_TICKS on keydown, decremented once per tick
// it fails to fire (not grounded and outside the coyote window), and zeroed
// the instant it does fire. Still gives one input edge at most one jump --
// buffering only extends *how long* an edge can wait to find its window,
// not how many jumps it can produce.
typedef struct InputState
{
    bool moveLeftPlayer1;
    bool moveRightPlayer1;
    bool jumpHeldPlayer1;
    bool shootHeldPlayer1;
    bool moveLeftPlayer2;
    bool moveRightPlayer2;
    bool jumpHeldPlayer2;
    bool shootHeldPlayer2;

    int jumpBufferTicksPlayer1;
    int jumpBufferTicksPlayer2;
} InputState;

// Bounded contact-event queue (Phase 24). Physics/contact detection may add
// events but must not score, despawn, damage, play sound, or transition a
// scene directly. The consequence phase consumes these entries once.
typedef enum
{
    GAME_EVENT_PROJECTILE_HIT,
    GAME_EVENT_ARCADE_PLAYER_HIT,
    GAME_EVENT_ARCADE_PLAYER_FELL,
    GAME_EVENT_ARCADE_ENEMY_ESCAPED,
    GAME_EVENT_ARCADE_BOSS_ESCAPED,
    GAME_EVENT_ARCADE_GAME_OVER_CHECK,
    GAME_EVENT_RUNNER_PLAYER_HIT,
    GAME_EVENT_RUNNER_PLAYER_FELL,
    GAME_EVENT_RUNNER_GAME_OVER_CHECK
} GameEventType;

typedef enum
{
    GAME_EVENT_TARGET_REGULAR_ENEMY,
    GAME_EVENT_TARGET_SMART_ENEMY,
    GAME_EVENT_TARGET_BOSS
} GameEventTarget;

typedef struct
{
    GameEventType type;
    GameEventTarget target;
    int projectileIndex;
    int targetIndex;
    bool secondPlayerProjectile;
} GameEvent;

typedef struct
{
    GameEvent events[MAX_GAME_EVENTS];
    int count;
} GameEventQueue;

typedef struct
{
    // scroll thw world
    float scrollX;
    // Presentation-only camera transform captured before each fixed tick.
    float prevScrollX;
    // Fraction of the pending fixed tick, written immediately before
    // rendering. Never read by physics or collision code.
    float renderAlpha;
    // Opt-in telemetry used only when ENDGAME_PERF_LOG is set. The counters
    // make the collision workload visible before further optimization work.
    bool perfLoggingEnabled;
    Uint64 perfProjectileActiveSamples;
    Uint64 perfProjectileTargetChecks;
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

    // Fixed pool (Phase 14) -- value arrays, not pointer arrays; see the
    // Bullet struct comment above and docs/projectile-correctness-map.md.
    Bullet bullets[MAX_BULLETS];
    Bullet secondBullets[MAX_BULLETS];

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

    // Application/platform state (renderer + scene) -- see AppContext above
    // and docs/gamestate-decomposition.md. Accessed as game->app.renderer /
    // game->app.scene.
    AppContext app;

    // Asset-group lifecycle flags (arcadeAssetsLoaded/runnerAssetsLoaded/
    // sharedAssetsLoaded) -- see AssetLifecycleFlags above and
    // docs/gamestate-decomposition.md. Each is set to true only after its
    // entire group's loads succeed (see arcade_assets_load/
    // runner_assets_load/shared_assets_load, src/loadGame.c), and reset to
    // false by the matching *_assets_unload(). sharedAssetsLoaded covers the
    // handful of textures/font both modes load identically (mult, leaders,
    // pause, brick, death, font) -- see docs/game-session-lifecycle.md.
    // Accessed as game->assetFlags.arcadeAssetsLoaded, etc.
    AssetLifecycleFlags assetFlags;

    // Edge-triggered, buffered jump-request countdowns -- see InputState
    // above, docs/input-simulation-separation-map.md, and
    // docs/game-feel-map.md. Accessed as game->input.jumpBufferTicksPlayer1/2.
    InputState input;

    // Per-fixed-tick contact events. Cleared before Arcade contact detection
    // and applied once in the consequence phase.
    GameEventQueue events;

    // DEPRECATED: superseded by `app.scene` (AppScene) above. No longer read or
    // written anywhere in active routing code as of the scene-state refactor;
    // kept declared (not deleted) per that phase's own scope rules. Safe to
    // remove entirely in a future phase.
    int menu_status;
    int menu0_status;
    int x_score;
    int x_list[25];
    int x_i;
    int kills_score;
    int kills_score_multi;
    int time, deathCountdown;
    int statusState;

    int iter;

} GameState;

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

// Render interpolation (Phase 20). Capture before each physics tick and
// synchronize after a reset or teleport to avoid rendering across a jump.
void capture_render_transforms(GameState *game);
void sync_render_transforms(GameState *game);
float render_lerp(float previous, float current, float alpha);

// Retained for direct collision verification; gameplay uses the complete
// capture_render_transforms() function above.
void capture_player_prev_y(GameState *game);
void consume_arcade_jump_requests(GameState *game);
void apply_arcade_player_forces(GameState *game, float dt);
void move_arcade_bullets(GameState *game, float dt);
void process(GameState *game, float dt);
void collisionDetect(GameState *game);
int processEvents(SDL_Window *window, GameState *game);
void doRender(SDL_Renderer *renderer, GameState *game);
void shutdown_status_lives (GameState *game);
void draw_status_lives(GameState *game);
void draw_status_lives2(GameState *game);
void init_status_lives(GameState *game);
int  doRender_menu1(SDL_Renderer *renderer, GameState *game);
int  doRender_menu2(SDL_Renderer *renderer, GameState *game);
int doRender_leaderboard(SDL_Renderer *renderer, GameState *game);
int doRender_pause(SDL_Renderer *renderer, GameState *game);
int collide2d(float x1, float y1, float x2, float y2, float wt1, float ht1, float wt2, float ht2);
void menu_events(GameState *gameState);
void load_menu1(GameState *game);
void load_menu2(GameState *game);

// Scene routing (src/scene.c) -- see docs/scene-state-map.md. Declared only in
// the focused inc/scene.h as of Phase 10 -- every caller (scene.c, main.c,
// menu_events.c, pause_events.c, processEvents.c) now includes it directly;
// see docs/gamestate-decomposition.md section 5 and
// docs/solid-gof-audit.md section 7.3 for the migration history.

// Per-mode frame pipeline (src/frame.c) -- see docs/frame-pipeline-map.md and
// inc/frame.h. Moved fully to inc/frame.h: arcade_frame()/runner_frame() have
// exactly one definer (src/frame.c) and one caller (src/main.c), so no other
// file needs this declaration from header.h.

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
void addBullet(GameState *game, float x, float y, float dx);
void addSecondBullet(GameState *game, float x, float y, float dx);
void removeSecondBullet(GameState *game, int i);

void init_status_kills(GameState *game);
void draw_status_kills(GameState *game);
void init_status_x(GameState *game);
void draw_status_x(GameState *game);
void init_status_x_list(GameState *game);

void load_menu0(GameState *game);
int  doRender_menu0(SDL_Renderer *renderer, GameState *game);
void menu0_events(GameState *gameState);
void collisionDetect2(GameState *game);
void doRender2(SDL_Renderer *renderer, GameState *game);
void consume_runner_jump_requests(GameState *game);
void apply_runner_player_forces(GameState *game, float dt);
void process2(GameState *game, float dt);
int processEvents2(SDL_Window *window, GameState *game);
int doRender_leaderboard2(SDL_Renderer *renderer, GameState *game);

void removeBullet(GameState *game, int i);

//Leo functions
int random_sign(int mult, int step);
