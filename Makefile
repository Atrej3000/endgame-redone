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

# Non-interactive Factory Method test: verifies enemy_spawn/smart_enemy_spawn/
# boss_spawn (src/entity_spawn.c) reproduce every one of the original 10
# duplicated inline call sites' exact field values, and that an out-of-range
# index is rejected without mutating anything -- a correctness guarantee the
# original inline code never had. See docs/verification/entity_spawn_test.c
# and docs/solid-gof-audit.md section 7.1.
mingw-entityspawntest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/entity_spawn_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/entityspawntest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/entityspawntest.exe

# Non-interactive Command-translation test: verifies translate_arcade_command/
# translate_runner_command/translate_menu_command (src/input_command.c) map
# every documented key to its documented GameCommand, including the
# dual-keycode menu pairs and the runner-only SDLK_0 cheat code exclusion.
# Pure functions -- no SDL subsystem init needed. See
# docs/verification/command_translation_test.c and
# docs/solid-gof-audit.md section 7.2.
mingw-commandtest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/command_translation_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/commandtest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/commandtest.exe

# Header self-containment check: each focused public header must compile
# standalone in an otherwise empty translation unit -- no reliance on some
# other header having been included first, no missing type, no implicit
# declaration. -fsyntax-only parses and typechecks without generating code
# or linking, since these files have no runtime behavior to exercise. See
# docs/verification/header_only_*.c and docs/gamestate-decomposition.md
# section 6.
mingw-headertest: $(BUILD_DIR)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_app.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_scene.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_frame.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_entity_spawn.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_input_command.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_input_snapshot.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_ai_forces.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	@echo "HEADER SELF-CONTAINMENT TEST: ALL PASS"

# Non-interactive GameState nested-struct grouping test: verifies
# AppContext (game->app.renderer/game->app.scene) and AssetLifecycleFlags
# (game->assetFlags.*) are correctly zero-initialized via calloc, explicitly
# writable through the real production functions, reset/cleaned-up
# correctly, and that unrelated GameState fields remain untouched. See
# docs/verification/gamestate_grouping_test.c and
# docs/gamestate-decomposition.md.
mingw-groupingtest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/gamestate_grouping_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/groupingtest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/groupingtest.exe

# Non-interactive fixed-timestep physics test: verifies process() (Arcade)
# reproduces the old per-frame code's exact numeric behavior at a fixed
# dt=1/60s, and that the same total simulated time chunked into different
# real-frame patterns (simulating different display refresh rates) produces
# identical final state and tick count -- the direct proof of Phase 11's
# refresh-rate-independence goal. See
# docs/verification/physics_timestep_test.c and
# docs/physics-timestep-map.md.
mingw-physicstest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/physics_timestep_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/physicstest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/physicstest.exe

# Verifies InputState's edge-triggered jump-request consumption
# (consume_arcade_jump_requests/consume_runner_jump_requests): one grounded
# request fires exactly one jump and clears itself, an airborne request is
# dropped rather than buffered, and Runner's jump impulse uses
# JUMP_SPEED_PER_SEC rather than the never-converted bare -10 found during
# this phase's audit. See docs/verification/input_state_test.c and
# docs/input-simulation-separation-map.md.
mingw-inputtest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/input_state_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/inputtest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/inputtest.exe

# Verifies player-vs-ledge collision correctness (Phase 13): walking off a
# ledge clears onLedge (the core "reset grounded each step" bug fix),
# resting continuously re-affirms onLedge=1 across multiple ticks (the
# crossing-based landing check handling the exact-rest case), normal
# falling-and-landing still works, and Runner's ceiling bump now clears
# onLedge instead of incorrectly re-grounding the player. See
# docs/verification/collision_correctness_test.c and
# docs/collision-correctness-map.md.
mingw-collisiontest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/collision_correctness_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/collisiontest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/collisiontest.exe

# Verifies projectile correctness (Phase 14): pool spawn/despawn (no
# malloc/free, other slots untouched), move-once-per-step (a bullet moves by
# exactly BULLET_SPEED_PER_TICK per call, not the old 113x-per-tick bug),
# swept collision (a bullet whose prevX->x path crosses a target registers a
# hit even when its end-of-tick position alone would miss), and an
# end-to-end process() regression check. See
# docs/verification/projectile_correctness_test.c and
# docs/projectile-correctness-map.md.
mingw-projectiletest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/projectile_correctness_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/projectiletest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/projectiletest.exe

# Verifies game feel (Phase 15): coyote time decaying naturally over
# COYOTE_TICKS real consume calls, jump buffering on the Runner side, and
# variable-jump-height release-cut behavior (including that the removed
# grounded-hold-thrust quirk is gone -- apply_*_player_forces() no longer
# touch dy at all outside a genuine release edge). See
# docs/verification/game_feel_test.c and docs/game-feel-map.md.
mingw-gamefeeltest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/game_feel_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/gamefeeltest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/gamefeeltest.exe

# Verifies input snapshot isolation (Phase 17): input_capture_arcade()/
# input_capture_runner() set exactly the right InputState fields from a
# fabricated key array; a simulated multi-tick real frame applies held
# movement on every tick but the release-cut only once; the pre-existing
# left-wins precedence on simultaneous left+right holds; player 1/2 field
# isolation; and session reset clearing every new field. See
# docs/verification/input_snapshot_test.c and
# docs/input-snapshot-architecture-map.md.
mingw-inputsnapshottest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/input_snapshot_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/inputsnapshottest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/inputsnapshottest.exe

# Dependency-light (stdlib-only) Python script. Override PYTHON on the
# command line if python3 isn't on PATH (e.g. `make PYTHON="py -3" audit-repo`
# on some Windows setups).
PYTHON := python3

# Repository usage integrity check: confirms every resource/-shaped string
# literal in src/*.c points at a file that exists with exact case, and every
# inc/header.h function prototype has a matching definition. See
# scripts/audit_repository_usage.py and docs/unused-code-assets-audit.md.
audit-repo:
	$(PYTHON) scripts/audit_repository_usage.py

mingw-clean:
	rm -rf $(BUILD_DIR)

# ---------------------------------------------------------------------------
# Additive Linux validation build (does not affect the macOS build above or
# the Windows/MinGW build below). Requires the SDL2/SDL2_image/SDL2_ttf/
# SDL2_mixer development packages installed via the system package manager
# (e.g. `apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
# libsdl2-mixer-dev`), discovered via pkg-config -- no vendoring needed,
# unlike the Windows/MinGW path, since Linux distributions package SDL2
# system-wide. LINUX_COMPAT_INCLUDE optionally points at a directory with
# SDL2_image/SDL_image.h-style forwarding headers -- this project's headers
# use that macOS-framework-style include path, and most Linux distributions
# are expected to ship the flat SDL2/SDL_image.h layout instead (the same
# mismatch already solved for MinGW; see scripts/setup-mingw-sdl2.sh). Empty
# by default; not needed on a system whose SDL2 packages happen to already
# provide the SDL2_image/-style layout.
# ---------------------------------------------------------------------------
CC_LINUX := gcc
LINUX_WARN_FLAGS := -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
	-Wsign-conversion -Wformat=2 -Wnull-dereference -Wdouble-promotion
LINUX_PKGS := sdl2 SDL2_image SDL2_ttf SDL2_mixer
LINUX_COMPAT_INCLUDE :=
LINUX_INCLUDES := -I $(INC) $(if $(LINUX_COMPAT_INCLUDE),-I $(LINUX_COMPAT_INCLUDE)) $(shell pkg-config --cflags $(LINUX_PKGS) 2>/dev/null)
# -lm is explicit (not just pulled in transitively via SDL2) because
# src/random_sign.c calls pow(): modern Ubuntu's ld defaults to
# --as-needed and no longer resolves a transitive libm dependency for you
# ("DSO missing from command line"), unlike the MinGW toolchain used above.
LINUX_LIBS := $(shell pkg-config --libs $(LINUX_PKGS) 2>/dev/null) -lm

LINUX_BUILD_DIR := build-linux
LINUX_EXEC := $(LINUX_BUILD_DIR)/endgame-linux

$(LINUX_BUILD_DIR):
	mkdir -p $(LINUX_BUILD_DIR)

linux: $(LINUX_BUILD_DIR)
	$(CC_LINUX) $(SRCS) $(LINUX_WARN_FLAGS) $(LINUX_INCLUDES) \
		-o $(LINUX_EXEC) $(LINUX_LIBS)

# Non-interactive smoke test on Linux -- reuses the exact same test source
# as the MinGW smoke test (MINGW_SRCS_NO_MAIN is just a source-file list,
# not MinGW-specific). See docs/verification/smoke_init_shutdown.c.
linux-smoketest: $(LINUX_BUILD_DIR)
	$(CC_LINUX) $(MINGW_SRCS_NO_MAIN) docs/verification/smoke_init_shutdown.c \
		$(LINUX_WARN_FLAGS) $(LINUX_INCLUDES) \
		-o $(LINUX_BUILD_DIR)/smoketest $(LINUX_LIBS)
	./$(LINUX_BUILD_DIR)/smoketest

linux-clean:
	rm -rf $(LINUX_BUILD_DIR)

.PHONY: all clean mingw mingw-dlls mingw-asan mingw-run mingw-smoketest mingw-scenetest mingw-lifecycletest mingw-frametest mingw-deathtest mingw-entityspawntest mingw-commandtest mingw-headertest mingw-groupingtest mingw-physicstest mingw-inputtest mingw-collisiontest mingw-projectiletest mingw-gamefeeltest mingw-inputsnapshottest print-mingw-versions audit-repo linux linux-smoketest linux-clean mingw-clean
