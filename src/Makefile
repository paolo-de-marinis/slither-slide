RIV_HOME ?= $(HOME)/.riv
RIVEMU ?= $(RIV_HOME)/rivemu
RIVEMU_SDK ?= $(RIV_HOME)/rivos-sdk.ext2
GAME_CPPFLAGS ?=
STRICT_BUILD_DIR ?= ../.strict-build

SOURCES = audio.c body_chain.c camera.c caterpillar_char.c char_selector.c collectible.c \
	collectible_render.c collision.c developer_controls.c game.c game_render.c levels.c main.c \
	room_layout.c scoring.c snake_char.c snake_geometry.c snake_motion.c spline_math.c \
	technical_view.c walls.c
OBJECTS = $(SOURCES:.c=.o)
STRICT_CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror -fanalyzer -I.

.PHONY: all clean run smoke strict test

all: snake.sqfs

%.o: %.c
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -no-window -sdk -workspace -mem=1024 \
		-exec "gcc $(GAME_CPPFLAGS) $< -c -o $@ \$$(riv-opt-flags -Ospeed)"

snake: $(OBJECTS)
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -no-window -sdk -workspace \
		-exec "gcc *.o -o snake -lriv -lm"
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -no-window -sdk -workspace \
		-exec "riv-strip snake"

snake.sqfs: snake songs/gameplay.rivcard
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -no-window -sdk -workspace \
		-exec "riv-mksqfs snake songs snake.sqfs"

run: snake.sqfs
	$(RIVEMU) -cartridge=snake.sqfs

smoke: snake.sqfs
	$(RIVEMU) -quiet -no-window -no-yield -stop-frame=180 -cartridge=snake.sqfs

strict:
	mkdir -p $(STRICT_BUILD_DIR)/release $(STRICT_BUILD_DIR)/debug $(STRICT_BUILD_DIR)/cheats
	@set -e; for source in $(SOURCES); do \
		object=$${source%.c}.o; \
		$(CC) $(STRICT_CFLAGS) -c $$source -o $(STRICT_BUILD_DIR)/release/$$object; \
		$(CC) $(STRICT_CFLAGS) -DDEBUG_MODE=1 -c $$source \
			-o $(STRICT_BUILD_DIR)/debug/$$object; \
		$(CC) $(STRICT_CFLAGS) -DCHEATS_ENABLED=1 -c $$source \
			-o $(STRICT_BUILD_DIR)/cheats/$$object; \
	done

test:
	$(CC) -std=c11 -Wall -Wextra -Wshadow -Werror -I. \
		../tests/test_room_layout.c room_layout.c \
		-o /tmp/slither-room-layout-test
	/tmp/slither-room-layout-test
	$(CC) -std=c11 -Wall -Wextra -Wshadow -Werror -I. \
		../tests/test_level_doors.c levels.c room_layout.c walls.c -lm \
		-o /tmp/slither-level-doors-test
	/tmp/slither-level-doors-test
	$(CC) -std=c11 -Wall -Wextra -Wshadow -Werror \
		-DRIV_IMPORT_CONTEXT -DRIV_API=extern -I. \
		../tests/test_scoring.c scoring.c -o /tmp/slither-scoring-test
	/tmp/slither-scoring-test
	$(CC) -std=c11 -Wall -Wextra -Wshadow -Werror -I. \
		../tests/test_body_chain.c body_chain.c -lm -o /tmp/slither-body-chain-test
	/tmp/slither-body-chain-test
	$(CC) -std=c11 -Wall -Wextra -Wshadow -Werror -I. \
		../tests/test_spline_math.c spline_math.c -lm -o /tmp/slither-spline-test
	/tmp/slither-spline-test
	$(CC) -std=c11 -Wall -Wextra -Wshadow -Werror -I. \
		../tests/test_snake_geometry.c snake_geometry.c spline_math.c -lm \
		-o /tmp/slither-snake-geometry-test
	/tmp/slither-snake-geometry-test
	$(CC) -std=c11 -Wall -Wextra -Wshadow -Werror -I. \
		../tests/test_developer_controls.c developer_controls.c \
		-o /tmp/slither-developer-controls-test
	/tmp/slither-developer-controls-test
	$(CC) -std=c11 -Wall -Wextra -Wshadow -Werror -I. \
		../tests/test_gameplay_events.c snake_motion.c \
		-o /tmp/slither-gameplay-events-test
	/tmp/slither-gameplay-events-test
	$(CC) -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror \
		-DRIV_IMPORT_CONTEXT -DRIV_API=extern -I. \
		../tests/test_collision.c collision.c walls.c room_layout.c -lm \
		-o /tmp/slither-collision-test
	/tmp/slither-collision-test

clean:
	rm -f $(OBJECTS) snake snake.sqfs
	rm -f /tmp/slither-room-layout-test /tmp/slither-level-doors-test \
		/tmp/slither-scoring-test /tmp/slither-body-chain-test /tmp/slither-spline-test \
		/tmp/slither-snake-geometry-test /tmp/slither-developer-controls-test \
		/tmp/slither-gameplay-events-test /tmp/slither-collision-test
	rm -f $(STRICT_BUILD_DIR)/release/*.o $(STRICT_BUILD_DIR)/debug/*.o \
		$(STRICT_BUILD_DIR)/cheats/*.o
	rmdir $(STRICT_BUILD_DIR)/release $(STRICT_BUILD_DIR)/debug \
		$(STRICT_BUILD_DIR)/cheats 2>/dev/null || true
	rmdir $(STRICT_BUILD_DIR) 2>/dev/null || true
