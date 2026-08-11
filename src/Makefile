RIV_HOME ?= $(HOME)/.riv
RIVEMU ?= $(RIV_HOME)/rivemu
RIVEMU_SDK ?= $(RIV_HOME)/rivos-sdk.ext2

SOURCES = audio.c camera.c caterpillar_char.c char_selector.c game.c levels.c \
	objects.c room_layout.c snake_char.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean run smoke test

all: snake.sqfs

%.o: %.c
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -no-window -sdk -workspace -mem=1024 \
		-exec "gcc $< -c -o $@ \$$(riv-opt-flags -Ospeed)"

snake: $(OBJECTS)
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -no-window -sdk -workspace \
		-exec "gcc *.o -o snake -lriv -lm"

snake.sqfs: snake songs/gameplay.rivcard
	RIVEMU_SDK=$(RIVEMU_SDK) $(RIVEMU) -no-window -sdk -workspace \
		-exec "riv-mksqfs snake songs snake.sqfs"

run: snake.sqfs
	$(RIVEMU) -cartridge=snake.sqfs

smoke: snake.sqfs
	$(RIVEMU) -quiet -no-window -no-yield -stop-frame=180 -cartridge=snake.sqfs

test:
	$(CC) -std=c11 -Wall -Wextra -Werror -I. ../tests/test_room_layout.c room_layout.c \
		-o /tmp/slither-room-layout-test
	/tmp/slither-room-layout-test
	$(CC) -std=gnu11 -Wall -Wextra -Werror -I. ../tests/test_level_doors.c \
		levels.c room_layout.c -o /tmp/slither-level-doors-test
	/tmp/slither-level-doors-test

clean:
	rm -f $(OBJECTS) snake snake.sqfs
