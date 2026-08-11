#include "room_layout.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    int roomX = -1;
    int roomY = -1;

    assert(getRoomPosition(1, &roomX, &roomY));
    assert(roomX == 0 && roomY == 0);
    assert(getRoomPosition(2, &roomX, &roomY));
    assert(roomX == 1 && roomY == 0);
    assert(getRoomPosition(3, &roomX, &roomY));
    assert(roomX == 1 && roomY == 1);
    assert(getRoomPosition(4, &roomX, &roomY));
    assert(roomX == 0 && roomY == 1);
    assert(getRoomPosition(5, &roomX, &roomY));
    assert(roomX == 0 && roomY == 2);
    assert(getRoomPosition(8, &roomX, &roomY));
    assert(roomX == 3 && roomY == 2);
    assert(getRoomPosition(9, &roomX, &roomY));
    assert(roomX == 3 && roomY == 1);
    assert(getRoomPosition(10, &roomX, &roomY));
    assert(roomX == 3 && roomY == 0);
    assert(getRoomPosition(11, &roomX, &roomY));
    assert(roomX == 2 && roomY == 0);
    assert(getRoomPosition(12, &roomX, &roomY));
    assert(roomX == 2 && roomY == 1);

    for (int level = 1; level < 12; level++) {
        assert(areLevelsAdjacent(level, level + 1));
        assert(!canTraverseLevels(level, level + 1, false));
        assert(canTraverseLevels(level, level + 1, true));
        assert(canTraverseLevels(level + 1, level, false));
    }

    assert(getLevelAtPosition(-1, 0) == 0);
    assert(getLevelAtPosition(ROOMS_X, 0) == 0);
    assert(!getRoomPosition(0, &roomX, &roomY));
    assert(!areLevelsAdjacent(1, 3));
    assert(!canTraverseLevels(1, 8, true));

    puts("room layout: ok");
    return 0;
}
