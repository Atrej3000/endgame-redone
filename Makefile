# A simple Makefile for compiling small SDL projects

# set the compiler
CC := clang
INC = inc


# set the compiler flags
MACOS_ARCH_FLAGS ?= -arch x86_64
FFLAGS = $(MACOS_ARCH_FLAGS) \
	-F ./resource/frameworks -framework SDL2 -rpath @executable_path/resource/frameworks \
	-F ./resource/frameworks -framework SDL2_image -rpath @executable_path/resource/frameworks \
	-F ./resource/frameworks -framework SDL2_ttf -rpath @executable_path/resource/frameworks \
	-F ./resource/frameworks -framework SDL2_mixer -rpath @executable_path/resource/frameworks

CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror
# All public headers participate in incremental rebuild decisions. Depending
# on the directory itself allowed edits to an existing header to leave a
# stale executable considered up to date.
HDRS := $(wildcard inc/*.h)

# add source files here
SRCS := src/*.c

# generate names of object files
# OBJS := $(SRCS:.c=.o)

# name of executable
EXEC := endgame

# default recipe
all: $(EXEC)

# recipe for building the final executable
$(EXEC): $(SRCS) $(HDRS) Makefile
	@$(CC) $(SRCS) $(CFLAGS) $(FFLAGS) -o $(EXEC) -I $(INC)
	@printf "\r\33[2K $(NAME)\033[33;1m\tcompile\n"

# recipe for building object files
#$(OBJS): $(@:.o=.c) $(HDRS) Makefile
#	$(CC) -o $@ $(@:.o=.c) -c $(CFLAGS)

# recipe to clean the workspace
clean:
	rm -f $(EXEC)
	@printf "Build output - \t\033[31;1mdeleted\033[0m\n"


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
	-Wsign-conversion -Wformat=2 -Wnull-dereference -Wdouble-promotion -Werror

# Sources minus main.c, for the non-interactive smoke-test harness below
# (which supplies its own main()).
MINGW_SRCS_NO_MAIN := $(filter-out src/main.c,$(wildcard src/*.c))

BUILD_DIR := build-mingw
MINGW_EXEC := $(BUILD_DIR)/endgame-mingw.exe

# Use Windows' Python launcher from PowerShell/CMD, while retaining the usual
# python3 command for MSYS2 and Linux. Callers can override PYTHON explicitly.
ifeq ($(OS),Windows_NT)
PYTHON ?= py -3
else
PYTHON ?= python3
endif

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Copy the runtime DLLs next to the built exe so it can actually run. Python's
# shutil.copy2 keeps this target usable from PowerShell, CMD, MSYS2, and Linux.
mingw-dlls: $(BUILD_DIR) scripts/copy_mingw_dlls.py
	$(PYTHON) scripts/copy_mingw_dlls.py \
		--mingw-root "$(MINGW_ROOT)" \
		--triplet "$(MINGW_TRIPLET)" \
		--sdl2-version "$(SDL2_VERSION)" \
		--sdl2-image-version "$(SDL2_IMAGE_VERSION)" \
		--sdl2-ttf-version "$(SDL2_TTF_VERSION)" \
		--sdl2-mixer-version "$(SDL2_MIXER_VERSION)" \
		--destination "$(BUILD_DIR)"

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

# A package destination is deliberately caller-supplied: the packager refuses
# source/build roots and --force only replaces a directory carrying its exact
# ownership marker. Keeping this path explicit also avoids silently creating a
# large untracked resource tree in the repository.
MINGW_PACKAGE_DIR ?=

mingw-package: mingw
	$(if $(strip $(MINGW_PACKAGE_DIR)),,$(error MINGW_PACKAGE_DIR is required (for example: /tmp/endgame-windows)))
	$(PYTHON) scripts/package_mingw.py \
		--destination "$(MINGW_PACKAGE_DIR)" --force

mingw-package-verify:
	$(if $(strip $(MINGW_PACKAGE_DIR)),,$(error MINGW_PACKAGE_DIR is required (for example: /tmp/endgame-windows)))
	$(PYTHON) scripts/package_mingw.py \
		--destination "$(MINGW_PACKAGE_DIR)" --verify

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
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_collision_pipeline.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_physics_body.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_display.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_settings.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_audio_assets.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_replay.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_arcade_waves.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_runner_segments.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_combat_feedback.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_atomic_file.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_fixed_step.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_leaderboard.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
	$(CC_MINGW) -fsyntax-only docs/verification/header_only_preferences.c $(MINGW_WARN_FLAGS) $(MINGW_INCLUDES)
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
# exactly BULLET_SPEED_PER_SEC * PHYSICS_DT per call, not the old 113x-per-tick bug),
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

# Verifies AI/forces separation (Phase 18): the first-ever assertions on
# boss drift, regular-enemy drift, and smart-enemy chase math (previously
# untested); smart_enemy_select_target() picking the nearer of two players
# (and always man in single-player); and that process() still delegates to
# the extracted functions correctly. See docs/verification/ai_forces_test.c
# and docs/ai-forces-separation-map.md.
mingw-aiforcestest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/ai_forces_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/aiforcestest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/aiforcestest.exe

# Verifies collision ordering (Phase 19): the first-ever direct-call
# assertions on Arcade's body-contact/reached-bottom/fall-off-screen
# hazards and game-over transition, and Runner's star-contact/fall-hazard
# and game-over transition -- previously only reachable via a full
# processEvents()/processEvents2() call with a real event loop. See
# docs/verification/collision_pipeline_test.c and
# docs/collision-ordering-map.md.
mingw-collisionorderingtest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/collision_pipeline_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/collisionorderingtest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/collisionorderingtest.exe

# Verifies render interpolation (Phase 20): previous transforms are captured
# for each moving renderable category, alpha is clamped, and interpolation is
# presentation-only. See docs/verification/render_interpolation_test.c.
mingw-interpolationtest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/render_interpolation_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/interpolationtest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/interpolationtest.exe

# Verifies Phase 21's remaining time-based movement: Runner's automatic
# multiplayer camera and sinusoidal traps. Bullet/enemy/boss/smart-enemy
# coverage remains in projectile_correctness_test and ai_forces_test.
mingw-physicsunitstest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/physics_units_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/physicsunitstest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/physicsunitstest.exe

# Verifies Phase 22's shared body/collider model and the migrated projectile
# sweep. See docs/verification/physics_body_test.c.
mingw-physicsbodytest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/physics_body_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/physicsbodytest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/physicsbodytest.exe

# Verifies Phase 23's common axis-separated static-world solver.
mingw-worldcollisiontest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/world_collision_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/worldcollisiontest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/worldcollisiontest.exe

# Verifies Phase 28's persisted display defaults and bounds validation without
# creating a window. Runtime resize/fullscreen integration is covered by the
# smoke test's app_init/app_shutdown cycle.
mingw-displaytest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/display_settings_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/displaytest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/displaytest.exe

# Verifies Phase 29's documented defaults, safe key-rebinding bounds, and
# reset-to-default behavior without touching the per-user settings file.
mingw-settingstest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/settings_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/settingstest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/settingstest.exe

# Verifies Phase 31's deterministic replay boundary: recorded seed, mode,
# and per-fixed-tick input replay to an identical state, while changed input
# or seed changes the expected authoritative result. It needs no SDL window,
# renderer, audio device, or loaded game assets.
mingw-replaytest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/replay_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/replaytest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/replaytest.exe

# Verifies Phase 32's table-driven Arcade progression: cadence, rest periods,
# boss telegraphing, and completion only after all active entities clear.
mingw-wavetest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/arcade_waves_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/wavetest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/wavetest.exe

# Verifies Phase 33's authored Runner segment catalog, its bounded terrain
# transitions, and alternating streaming-buffer extensions.
mingw-segmenttest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/runner_segments_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/segmenttest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/segmenttest.exe

# Verifies Phase 34's deterministic visual-feedback state, bounded hit-stop,
# optional shake, particles, and per-entity animation clocks.
mingw-feedbacktest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/combat_feedback_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/feedbacktest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/feedbacktest.exe

# Pre-Phase-35 whole-game gap-audit gates. These deliberately link the real
# production sources (minus main.c) under the same strict warning policy as
# every established MinGW check.
mingw-prephase35-statetest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_state_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/prephase35-statetest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/prephase35-statetest.exe

mingw-prephase35-simulationtest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_simulation_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/prephase35-simulationtest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/prephase35-simulationtest.exe

mingw-prephase35-lifecycletest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_lifecycle_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/prephase35-lifecycletest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/prephase35-lifecycletest.exe

mingw-prephase35-integrationtest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_integration_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/prephase35-integrationtest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/prephase35-integrationtest.exe

# Persistence is already covered by the expanded settings/display harnesses;
# this named audit gate composes those real checks instead of duplicating them.
mingw-prephase35-persistencetest: mingw-settingstest mingw-displaytest
	@echo "PRE-PHASE35 PERSISTENCE TEST: ALL PASS"

mingw-prephase35-uitest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_ui_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/prephase35-uitest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/prephase35-uitest.exe

mingw-prephase35-robustnesstest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_robustness_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/prephase35-robustnesstest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/prephase35-robustnesstest.exe

mingw-prephase35-collisionaitest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_collision_ai_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/prephase35-collisionaitest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/prephase35-collisionaitest.exe

mingw-prephase35-soaktest: $(BUILD_DIR) mingw-dlls
	$(CC_MINGW) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_soak_test.c \
		$(MINGW_WARN_FLAGS) $(MINGW_INCLUDES) $(MINGW_LIBDIRS) \
		-o $(BUILD_DIR)/prephase35-soaktest.exe $(MINGW_LIBS)
	./$(BUILD_DIR)/prephase35-soaktest.exe

mingw-prephase35: mingw-prephase35-statetest \
	mingw-prephase35-simulationtest \
	mingw-prephase35-lifecycletest \
	mingw-prephase35-integrationtest \
	mingw-prephase35-persistencetest \
	mingw-prephase35-uitest \
	mingw-prephase35-robustnesstest \
	mingw-prephase35-collisionaitest \
	mingw-prephase35-soaktest

# Repository usage integrity check: confirms every resource/-shaped string
# literal in src/*.c points at a file that exists with exact case, and every
# inc/header.h function prototype has a matching definition. See
# scripts/audit_repository_usage.py and docs/unused-code-assets-audit.md.
audit-repo:
	$(PYTHON) scripts/test_audit_repository_usage.py
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
LINUX_WARN_FLAGS := -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
	-Wsign-conversion -Wformat=2 -Wnull-dereference -Wdouble-promotion -Werror
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

# Required Linux ASan/UBSan verification. Replay plus the pre-Phase-35
# simulation and integration gates exercise deterministic, headless production
# paths without relying on a display or audio device.
LINUX_SANITIZER_FLAGS := $(LINUX_WARN_FLAGS) -g -O1 \
	-fsanitize=address,undefined -fno-omit-frame-pointer

linux-asan-replay: $(LINUX_BUILD_DIR)
	$(CC_LINUX) $(MINGW_SRCS_NO_MAIN) docs/verification/replay_test.c \
		$(LINUX_SANITIZER_FLAGS) $(LINUX_INCLUDES) \
		-o $(LINUX_BUILD_DIR)/replay-asan $(LINUX_LIBS)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
		./$(LINUX_BUILD_DIR)/replay-asan

linux-asan-simulation: $(LINUX_BUILD_DIR)
	$(CC_LINUX) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_simulation_test.c \
		$(LINUX_SANITIZER_FLAGS) $(LINUX_INCLUDES) \
		-o $(LINUX_BUILD_DIR)/prephase35-simulation-asan $(LINUX_LIBS)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
		./$(LINUX_BUILD_DIR)/prephase35-simulation-asan

linux-asan-integration: $(LINUX_BUILD_DIR)
	$(CC_LINUX) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_integration_test.c \
		$(LINUX_SANITIZER_FLAGS) $(LINUX_INCLUDES) \
		-o $(LINUX_BUILD_DIR)/prephase35-integration-asan $(LINUX_LIBS)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
		./$(LINUX_BUILD_DIR)/prephase35-integration-asan

linux-asan-robustness: $(LINUX_BUILD_DIR)
	$(CC_LINUX) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_robustness_test.c \
		$(LINUX_SANITIZER_FLAGS) $(LINUX_INCLUDES) \
		-o $(LINUX_BUILD_DIR)/prephase35-robustness-asan $(LINUX_LIBS)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
		./$(LINUX_BUILD_DIR)/prephase35-robustness-asan

linux-asan-collision-ai: $(LINUX_BUILD_DIR)
	$(CC_LINUX) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_collision_ai_test.c \
		$(LINUX_SANITIZER_FLAGS) $(LINUX_INCLUDES) \
		-o $(LINUX_BUILD_DIR)/prephase35-collision-ai-asan $(LINUX_LIBS)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
		./$(LINUX_BUILD_DIR)/prephase35-collision-ai-asan

linux-asan-soak: $(LINUX_BUILD_DIR)
	$(CC_LINUX) $(MINGW_SRCS_NO_MAIN) docs/verification/pre_phase35_soak_test.c \
		$(LINUX_SANITIZER_FLAGS) $(LINUX_INCLUDES) \
		-o $(LINUX_BUILD_DIR)/prephase35-soak-asan $(LINUX_LIBS)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
		./$(LINUX_BUILD_DIR)/prephase35-soak-asan

linux-asan: linux-asan-replay linux-asan-simulation linux-asan-integration linux-asan-robustness linux-asan-collision-ai linux-asan-soak

linux-clean:
	rm -rf $(LINUX_BUILD_DIR)

.PHONY: all clean mingw mingw-dlls mingw-asan mingw-run mingw-package mingw-package-verify mingw-smoketest mingw-scenetest mingw-lifecycletest mingw-deathtest mingw-entityspawntest mingw-commandtest mingw-headertest mingw-groupingtest mingw-physicstest mingw-inputtest mingw-collisiontest mingw-projectiletest mingw-gamefeeltest mingw-inputsnapshottest mingw-aiforcestest mingw-collisionorderingtest mingw-interpolationtest mingw-physicsunitstest mingw-physicsbodytest mingw-worldcollisiontest mingw-displaytest mingw-settingstest mingw-replaytest mingw-wavetest mingw-segmenttest mingw-feedbacktest mingw-prephase35-statetest mingw-prephase35-simulationtest mingw-prephase35-lifecycletest mingw-prephase35-integrationtest mingw-prephase35-persistencetest mingw-prephase35-uitest mingw-prephase35-robustnesstest mingw-prephase35-collisionaitest mingw-prephase35-soaktest mingw-prephase35 print-mingw-versions audit-repo linux linux-smoketest linux-asan-replay linux-asan-simulation linux-asan-integration linux-asan-robustness linux-asan-collision-ai linux-asan-soak linux-asan linux-clean mingw-clean
