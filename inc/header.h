#pragma once

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
#include <malloc/malloc.h>
#include <limits.h>

#define STATUS_STATE_LIVES 0
#define STATUS_STATE_GAME 1
#define STATUS_STATE_GAMEOVER 2
#define WIDTH 1280
#define HEIGHT 720
#define GRAVITY 0.5f
#define MAX_BULLETS 1000

#define NUM_STARS 100
#define NUM_ENEMIES 101
#define NUM_SMART_ENEMIES 10
#define LEADERBOARD_TXT "./resource/text/Fonts/leaderboard"

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
    int onLedge, isDead, shooting;//, isdead, visible, countShots;

    int animFrame, animFrameSecond, currentSpriteIdle, currentSpriteRun, currentSpriteRun2,
        currentSpriteJump, currentSpriteJump2, currentSpriteAttack1, currentSpriteSkill,
        facingLeft, slowingDown, visible0, visible2, attack;
    SDL_Texture *sheetTextureIdle, *sheetTextureRun,
            *sheetTextureRun2, *sheetTextureJump, *sheetTextureJump2, *sheetTextureAttack1, *sheetTextureSkill;
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

typedef struct
{
    // scroll thw world 
    float scrollX;
    int multiPlayer;

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
    SDL_Texture *secondPlayerImage;
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
    SDL_Texture *enemyFrame;
    SDL_Texture *brick_block;
    SDL_Texture *copper_block;
    SDL_Texture *background;
    SDL_Texture *bulletTexture;
    SDL_Texture *secondBulletTexture;
    SDL_Texture *enemyTexture2;
    SDL_Texture *bossTexture;
    int labelW, labelH;

    //Background
    int CurrentSpriteBack, CurrentSpriteBack2, CurrentSpriteBack3;
    SDL_Texture *sheetTextureBack, *sheetTextureBack2, *sheetTextureSun;

    // Fonts
    TTF_Font *font;

    // FOR WAVES
    int countWaves;

    //SOUNDS
    int musicChannel;
    Mix_Music *menuMus, *battleMus, *runnerMus;
    Mix_Chunk *jumpSound, *kickSound, *select, *shootSound, *damageSound;

    //Renderer
    SDL_Renderer *renderer;

    int menu_status;
    int menu0_status;
    int x_score;
    int x_list[25];
    //char *x_names[20];
    int x_i;
    int kills_score;
    int kills_list[25];
    int kills_score_multi;
    int time, deathCountdown;
    int statusState;

    int iter;

    //SDL_Event event;

} GameState;

    //something for leaderboard
//    int leaderboard_list[25] = {0};
//    int *leader_list = leaderboard_list;clear

//enum for menu prototypes
enum menu_buttons{
    MENU,
    SINGLEPLAYER,
    MULTIPLAYER,
    LEADERBOARD,
    EXIT,
    RUNNER
};

// PRototypes
void loadGame(GameState *game);
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
void shutdown_status_kills (GameState *game);
void init_status_x_list(GameState *game);
void draw_status_x_list(GameState *game);

//for two mods game
void load_menu0(GameState *game);
int  doRender_menu0(SDL_Renderer *renderer, GameState *game);
void menu0_events(GameState *gameState);
void loadGame2(GameState *game);
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
