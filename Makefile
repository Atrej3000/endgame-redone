# A simple Makefile for compiling small SDL projects

# set the compiler
CC := clang
INC = inc


# set the compiler flags
FFLAGS = -F ./resource/frameworks -framework SDL2 -rpath ./resource/frameworks \
	 -F ./resource/frameworks -framework SDL2_image -rpath ./resource/frameworks \
	 -F ./resource/frameworks -framework SDL2_ttf -rpath ./resource/frameworks \
	 -F ./resource/frameworks -framework SDL2_mixer -rpath ./resource/frameworks \

CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror
# add header files here
HDRS := inc/header.h \

# add source files here
SRCS := src/*.c
		#src/main.c \
		#src/status.c

# generate names of object files
# OBJS := $(SRCS:.c=.o)

# name of executable
EXEC := endgame

# default recipe
all: $(EXEC)

# recipe for building the final executable
$(EXEC): $(SRCS) $(INC) Makefile
	@$(CC) $(SRCS) $(CFLAGS) $(FFLAGS) -o $(EXEC) -I $(INC)
	@printf "\r\33[2K $(NAME)\033[33;1m\tcompile\n"

# recipe for building object files
#$(OBJS): $(@:.o=.c) $(HDRS) Makefile
#	$(CC) -o $@ $(@:.o=.c) -c $(CFLAGS)

# recipe to clean the workspace
clean:		
	rm -f $(OBJS)
	@printf "Object files - \t\033[31;1mdeleted\033[0m\n"


uninstall: clean
	rm -f $(EXEC)
	@printf "$(NAME)\t\033[31;1muninstalled\n"

reinstall: uninstall all

run: all
	./$(EXEC)

# ---------------------------------------------------------------------------
# Additive Windows/MinGW validation build (does not affect the macOS build
# above). Requires the SDL2 MinGW "devel" packages for SDL2/SDL2_image/
# SDL2_ttf/SDL2_mixer, vendored locally and gitignored under ./vendor/.
# See docs/refactor-plan.md for the exact fetch/setup steps.
# ---------------------------------------------------------------------------
CC_MINGW := gcc
MINGW_ROOT := vendor/SDL2-mingw
MINGW_TRIPLET := x86_64-w64-mingw32

# Pinned SDL2 MinGW "devel" package versions -- the single source of truth for
# these numbers. scripts/setup-mingw-sdl2.sh reads them back via
# `make print-mingw-versions` rather than hardcoding a third copy, so the
# fetch/extract step and these build flags can never drift out of sync.
SDL2_VERSION := 2.32.10
SDL2_IMAGE_VERSION := 2.8.12
SDL2_TTF_VERSION := 2.24.0
SDL2_MIXER_VERSION := 2.8.2

MINGW_INCLUDES := \
	-I $(INC) \
	-I $(MINGW_ROOT)/compat-include \
	-I $(MINGW_ROOT)/SDL2-$(SDL2_VERSION)/$(MINGW_TRIPLET)/include \
	-I $(MINGW_ROOT)/SDL2-$(SDL2_VERSION)/$(MINGW_TRIPLET)/include/SDL2 \
	-I $(MINGW_ROOT)/SDL2_image-$(SDL2_IMAGE_VERSION)/$(MINGW_TRIPLET)/include \
	-I $(MINGW_ROOT)/SDL2_image-$(SDL2_IMAGE_VERSION)/$(MINGW_TRIPLET)/include/SDL2 \
	-I $(MINGW_ROOT)/SDL2_ttf-$(SDL2_TTF_VERSION)/$(MINGW_TRIPLET)/include \
	-I $(MINGW_ROOT)/SDL2_ttf-$(SDL2_TTF_VERSION)/$(MINGW_TRIPLET)/include/SDL2 \
	-I $(MINGW_ROOT)/SDL2_mixer-$(SDL2_MIXER_VERSION)/$(MINGW_TRIPLET)/include \
	-I $(MINGW_ROOT)/SDL2_mixer-$(SDL2_MIXER_VERSION)/$(MINGW_TRIPLET)/include/SDL2

MINGW_LIBDIRS := \
	-L $(MINGW_ROOT)/SDL2-$(SDL2_VERSION)/$(MINGW_TRIPLET)/lib \
	-L $(MINGW_ROOT)/SDL2_image-$(SDL2_IMAGE_VERSION)/$(MINGW_TRIPLET)/lib \
	-L $(MINGW_ROOT)/SDL2_ttf-$(SDL2_TTF_VERSION)/$(MINGW_TRIPLET)/lib \
	-L $(MINGW_ROOT)/SDL2_mixer-$(SDL2_MIXER_VERSION)/$(MINGW_TRIPLET)/lib

MINGW_LIBS := -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

MINGW_WARN_FLAGS := -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
	-Wsign-conversion -Wformat=2 -Wnull-dereference -Wdouble-promotion

# Sources minus main.c, for the non-interactive smoke-test harness below
# (which supplies its own main()).
MINGW_SRCS_NO_MAIN := $(filter-out src/main.c,$(wildcard src/*.c))

BUILD_DIR := build-mingw
MINGW_EXEC := $(BUILD_DIR)/endgame-mingw.exe

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Copy the runtime DLLs next to the built exe so it can actually run.
mingw-dlls: $(BUILD_DIR)
	cp $(MINGW_ROOT)/SDL2-$(SDL2_VERSION)/$(MINGW_TRIPLET)/bin/SDL2.dll $(BUILD_DIR)/
	cp $(MINGW_ROOT)/SDL2_image-$(SDL2_IMAGE_VERSION)/$(MINGW_TRIPLET)/bin/SDL2_image.dll $(BUILD_DIR)/
	cp $(MINGW_ROOT)/SDL2_ttf-$(SDL2_TTF_VERSION)/$(MINGW_TRIPLET)/bin/SDL2_ttf.dll $(BUILD_DIR)/
	cp $(MINGW_ROOT)/SDL2_mixer-$(SDL2_MIXER_VERSION)/$(MINGW_TRIPLET)/bin/SDL2_mixer.dll $(BUILD_DIR)/

# Machine-readable version dump for scripts/setup-mingw-sdl2.sh to consume,
# so the fetch/extract logic never has its own hardcoded copy of these
# numbers -- this Makefile is the single source of truth.
print-mingw-versions:
	@echo "SDL2_VERSION=$(SDL2_VERSION)"
	@echo "SDL2_IMAGE_VERSION=$(SDL2_IMAGE_VERSION)"
	@echo "SDL2_TTF_VERSION=$(SDL2_TTF_VERSION)"
	@echo "SDL2_MIXER_VERSION=$(SDL2_MIXER_VERSION)"

mingw: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(SRCS) $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(MINGW_EXEC) $(MINGW_LIBS)

# Debug build with AddressSanitizer + UndefinedBehaviorSanitizer.
# NOTE: as of this writing, neither MinGW-w64 GCC distribution available in
# this environment (scoop 'gcc'/nuwen, or scoop 'mingw-winlibs'/WinLibs) ships
# libasan/libubsan for the x86_64-w64-mingw32 target, so this target currently
# fails to link with "cannot find -lasan"/"-lubsan". This is a documented,
# confirmed toolchain gap (ASan/UBSan on native Windows generally needs the
# MSVC toolchain, not mingw), not something disabled or worked around here.
# Kept as correctly-specified, ready-to-use infrastructure for whenever a
# sanitizer-capable toolchain (Linux gcc/clang, or clang-cl + MSVC) is used.
mingw-asan: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(SRCS) $(MINGW_WARN_FLAGS) -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer \
		$(MINGW_INCLUDES) $(MINGW_LIBDIRS) -o $(BUILD_DIR)/endgame-mingw-asan.exe $(MINGW_LIBS)

mingw-run: mingw
	./$(MINGW_EXEC)

# Non-interactive runtime smoke test: exercises the real app_init -> one real
# asset-load pass -> app_shutdown -> repeated app_shutdown cycle end to end,
# without needing keyboard/window input injection. See
# docs/verification/smoke_init_shutdown.c and docs/refactor-log.md.
mingw-smoketest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/smoke_init_shutdown.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/smoketest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/smoketest.exe

# Non-interactive scene-transition test: exercises app_change_scene() and its
# enter-hooks without needing keyboard/window input injection. See
# docs/verification/scene_transition_test.c and docs/scene-state-map.md.
mingw-scenetest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/scene_transition_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/scenetest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/scenetest.exe

# Non-interactive asset/session lifecycle test: exercises
# arcade_assets_load/unload, runner_assets_load/unload,
# arcade_session_reset/runner_session_reset, and the pause/new-game
# transition sequences directly. See docs/verification/lifecycle_test.c
# and docs/game-session-lifecycle.md.
mingw-lifecycletest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/lifecycle_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/lifecycletest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/lifecycletest.exe

# Non-interactive frame-pipeline test: exercises the real doRender/doRender2/
# process/process2/processEvents/processEvents2/arcade_frame/runner_frame/
# pause functions directly -- render purity, the relocated death mutation,
# the double-transition guard, animation wrap/idle, and pause purity. See
# docs/verification/frame_pipeline_test.c and docs/frame-pipeline-map.md.
mingw-frametest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/frame_pipeline_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/frametest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/frametest.exe

# Non-interactive Runner death-lifecycle test: exercises the real
# runner_trigger_death/runner_update_death/runner_frame/process2/collisionDetect2/
# processEvents2/doRender2/runner_session_reset functions directly -- single
# death, repeated trigger, game over, pause, session reset, scene departure,
# render purity, and the left-edge/multiplayer respawn fixes. See
# docs/verification/runner_death_test.c and docs/runner-death-lifecycle.md.
mingw-deathtest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/runner_death_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/deathtest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/deathtest.exe

mingw-clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean mingw mingw-dlls mingw-asan mingw-run mingw-smoketest mingw-scenetest mingw-lifecycletest mingw-frametest mingw-deathtest print-mingw-versions mingw-clean
