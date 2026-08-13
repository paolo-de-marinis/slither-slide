RIV_HOME ?= $(HOME)/.riv
RIVEMU ?= $(RIV_HOME)/rivemu
RIVEMU_SDK ?= $(RIV_HOME)/rivos-sdk.ext2

SOURCES = audio.c body_chain.c camera.c caterpillar_char.c char_selector.c \
	collectible.c collectible_render.c collision.c game.c game_render.c levels.c main.c room_layout.c \
	scoring.c snake_char.c snake_motion.c spline_math.c walls.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean run smoke strict test

all: snake.sqfs

%.o: %.c
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -no-window -sdk -workspace -mem=1024 \
		-exec "gcc $< -c -o $@ \$$(riv-opt-flags -Ospeed)"

snake: $(OBJECTS)
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -no-window -sdk -workspace \
		-exec "gcc $(OBJECTS) -o snake -lriv -lm"

snake.sqfs: snake songs/gameplay.rivcard
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -no-window -sdk -workspace \
		-exec "riv-mksqfs snake songs snake.sqfs"

run: snake.sqfs
	$(RIVEMU) -cartridge=snake.sqfs

smoke: snake.sqfs
	$(RIVEMU) -quiet -no-window -no-yield -stop-frame=180 -cartridge=snake.sqfs

strict:
	$(CC) -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror -fanalyzer -I. \
		-fsyntax-only $(SOURCES)
	$(CC) -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror -fanalyzer \
		-DDEBUG_MODE=1 -I. -fsyntax-only $(SOURCES)

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
		../tests/test_gameplay_events.c snake_motion.c \
		-o /tmp/slither-gameplay-events-test
	/tmp/slither-gameplay-events-test

clean:
	rm -f $(OBJECTS) snake snake.sqfs
