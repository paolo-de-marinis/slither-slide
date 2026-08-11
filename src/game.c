#include "seqt.h"
#include "audio.h"
#include "riv.h"
#include "game.h"
#include "levels.h"
#include "char_selector.h"
#include "snake_char.h"
#include "caterpillar_char.h"
#include "ui_constants.h"
#include "objects.h"
#include "camera.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static bool checkCollision(riv_vec2i nextHeadPosition);
static bool collidesWithRoomEdge(float headX, float headY, float headRadius, int roomX, int roomY);
static bool
collidesWithWall(riv_vec2i nextHeadPosition, float headX, float headY, float headRadius);
static bool collidesWithBody(float headX, float headY, float headRadius);
static void updateJoints(void);
static void followPreviousJoint(int jointIndex);
static void separateDistantJoints(int jointIndex);
static void pushJointFromWalls(int jointIndex);
static void updateScores(void);
static int buildLevelDetails(char *details, size_t detailsSize);
static void drawGameOver(void);
static void drawGameCompleted(void);

GameData game;

static bool checkCollision(riv_vec2i nextHeadPosition) {
    if (nextHeadPosition.x < 0 || nextHeadPosition.x >= WORLD_TILES_X || nextHeadPosition.y < 0 ||
        nextHeadPosition.y >= WORLD_TILES_Y) {
        return true;
    }

    int roomX = 0;
    int roomY = 0;
    if (!getRoomPosition(getCurrentLevel(), &roomX, &roomY)) {
        return true;
    }

    float headX = nextHeadPosition.x * TILE_SIZE + TILE_SIZE / 2;
    float headY = nextHeadPosition.y * TILE_SIZE + TILE_SIZE / 2;
    float headRadius = getSnakeBodyWidth(0) * 0.67f;

    if (collidesWithRoomEdge(headX, headY, headRadius, roomX, roomY)) {
        return true;
    }
    if (collidesWithWall(nextHeadPosition, headX, headY, headRadius)) {
        return true;
    }
    return collidesWithBody(headX, headY, headRadius);
}

static bool collidesWithRoomEdge(float headX, float headY, float headRadius, int roomX, int roomY) {
    float roomLeft = roomX * ROOM_WIDTH;
    float roomRight = (roomX + 1) * ROOM_WIDTH;
    float roomTop = roomY * ROOM_HEIGHT;
    float roomBottom = (roomY + 1) * ROOM_HEIGHT;
    float horizontalDoorStart =
        roomTop + DOOR_START_SEGMENT * ROOM_HEIGHT / (float)WALL_SEGMENTS_PER_SIDE;
    float horizontalDoorEnd = roomTop + (DOOR_START_SEGMENT + DOOR_SEGMENT_COUNT) * ROOM_HEIGHT /
                                            (float)WALL_SEGMENTS_PER_SIDE;
    float verticalDoorStart =
        roomLeft + DOOR_START_SEGMENT * ROOM_WIDTH / (float)WALL_SEGMENTS_PER_SIDE;
    float verticalDoorEnd = roomLeft + (DOOR_START_SEGMENT + DOOR_SEGMENT_COUNT) * ROOM_WIDTH /
                                           (float)WALL_SEGMENTS_PER_SIDE;
    bool inHorizontalDoor =
        headY >= horizontalDoorStart - headRadius && headY <= horizontalDoorEnd + headRadius;
    bool inVerticalDoor =
        headX >= verticalDoorStart - headRadius && headX <= verticalDoorEnd + headRadius;

    if (headX < roomLeft + WALL_THICKNESS) {
        int nextLevel = getLevelAtPosition(roomX - 1, roomY);
        if (inHorizontalDoor && canTransitionToLevel(nextLevel)) {
            return false;
        }
        return true;
    }

    if (headX > roomRight - WALL_THICKNESS) {
        int nextLevel = getLevelAtPosition(roomX + 1, roomY);
        if (inHorizontalDoor && canTransitionToLevel(nextLevel)) {
            return false;
        }
        return true;
    }

    if (headY < roomTop + WALL_THICKNESS) {
        int nextLevel = getLevelAtPosition(roomX, roomY - 1);
        if (inVerticalDoor && canTransitionToLevel(nextLevel)) {
            return false;
        }
        return true;
    }

    if (headY > roomBottom - WALL_THICKNESS) {
        int nextLevel = getLevelAtPosition(roomX, roomY + 1);
        if (inVerticalDoor && canTransitionToLevel(nextLevel)) {
            return false;
        }
        return true;
    }

    return false;
}

static bool
collidesWithWall(riv_vec2i nextHeadPosition, float headX, float headY, float headRadius) {
    for (int i = 0; i < object.wallCount; i++) {
        Wall *wall = &object.walls[i];

        if (wall->width == 0 || wall->height == 0) {
            continue;
        }

        float closestX = fmaxf(wall->x, fminf(headX, wall->x + wall->width));
        float closestY = fmaxf(wall->y, fminf(headY, wall->y + wall->height));
        float dx = headX - closestX;
        float dy = headY - closestY;
        float distanceSquared = dx * dx + dy * dy;

        if (distanceSquared < (headRadius * headRadius)) {
            if (DEBUG_MODE) {
                riv_printf("Game Over: Wall collision at position (%d, %d)\n",
                           nextHeadPosition.x,
                           nextHeadPosition.y);
            }
            return true;
        }
    }
    return false;
}

static bool collidesWithBody(float headX, float headY, float headRadius) {
    for (int i = 4; i < game.jointCount - 1; i++) {
        float dx = headX - game.joints[i].x;
        float dy = headY - game.joints[i].y;
        float distanceSquared = dx * dx + dy * dy;

        float collisionRadius = getSnakeBodyWidth(i) + headRadius;
        if (distanceSquared < (collisionRadius * collisionRadius)) {
            if (DEBUG_MODE) {
                riv_printf("Game Over: Self collision at joint %d\n", i);
            }
            return true;
        }

        float segmentSteps = 4.0f;
        for (float t = 0.0f; t <= 1.0f; t += 1.0f / segmentSteps) {
            float segmentX = game.joints[i - 1].x * (1.0f - t) + game.joints[i].x * t;
            float segmentY = game.joints[i - 1].y * (1.0f - t) + game.joints[i].y * t;

            dx = headX - segmentX;
            dy = headY - segmentY;
            distanceSquared = dx * dx + dy * dy;

            if (distanceSquared < (collisionRadius * collisionRadius)) {
                if (DEBUG_MODE) {
                    riv_printf("Game Over: Self collision with segment %d\n", i);
                }
                return true;
            }
        }
    }

    return false;
}

static void updateJoints(void) {
    game.joints[0].x = game.headPosition.x * TILE_SIZE + TILE_SIZE / 2;
    game.joints[0].y = game.headPosition.y * TILE_SIZE + TILE_SIZE / 2;
    game.joints[0].angle = atan2f(game.headDirection.y, game.headDirection.x);

    for (int i = 1; i < game.jointCount; i++) {
        followPreviousJoint(i);
        separateDistantJoints(i);
        pushJointFromWalls(i);
    }
}

static void followPreviousJoint(int jointIndex) {
    float deltaX = game.joints[jointIndex - 1].x - game.joints[jointIndex].x;
    float deltaY = game.joints[jointIndex - 1].y - game.joints[jointIndex].y;
    float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);

    if (distance <= NORMAL_EPSILON) {
        return;
    }

    float moveFactor = 0.0f;
    if (distance > TILE_SIZE) {
        moveFactor = (distance - TILE_SIZE) / distance;
    } else {
        moveFactor = (distance - (TILE_SIZE * 0.9f)) / distance * 0.5f;
    }

    game.joints[jointIndex].x += deltaX * moveFactor;
    game.joints[jointIndex].y += deltaY * moveFactor;
    game.joints[jointIndex].angle = atan2f(deltaY, deltaX);
}

static void separateDistantJoints(int jointIndex) {
    // Current levels stay small enough for a direct pair comparison.
    for (int otherIndex = 0; otherIndex < game.jointCount; otherIndex++) {
        if (abs(jointIndex - otherIndex) <= 2) {
            continue;
        }

        float deltaX = game.joints[jointIndex].x - game.joints[otherIndex].x;
        float deltaY = game.joints[jointIndex].y - game.joints[otherIndex].y;
        float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);
        float combinedWidth = getSnakeBodyWidth(jointIndex) + getSnakeBodyWidth(otherIndex);
        float minimumDistance = combinedWidth * 0.8f;
        if (distance >= minimumDistance || distance <= 0.0f) {
            continue;
        }

        float pushX = deltaX / distance;
        float pushY = deltaY / distance;
        float pushAmount = (minimumDistance - distance) * 0.5f;
        game.joints[jointIndex].x += pushX * pushAmount;
        game.joints[jointIndex].y += pushY * pushAmount;
        game.joints[otherIndex].x -= pushX * pushAmount;
        game.joints[otherIndex].y -= pushY * pushAmount;

        for (int offset = 1; offset < 3; offset++) {
            if (jointIndex + offset < game.jointCount) {
                game.joints[jointIndex + offset].x += pushX * pushAmount * (0.5f / offset);
                game.joints[jointIndex + offset].y += pushY * pushAmount * (0.5f / offset);
            }
            if (otherIndex + offset < game.jointCount) {
                game.joints[otherIndex + offset].x -= pushX * pushAmount * (0.5f / offset);
                game.joints[otherIndex + offset].y -= pushY * pushAmount * (0.5f / offset);
            }
        }
    }
}

static void pushJointFromWalls(int jointIndex) {
    float collisionWidth = getSnakeBodyWidth(jointIndex);

    for (int wallIndex = 0; wallIndex < object.wallCount; wallIndex++) {
        Wall *wall = &object.walls[wallIndex];
        if (wall->width == 0 || wall->height == 0) {
            continue;
        }

        float closestX = fmaxf(wall->x, fminf(game.joints[jointIndex].x, wall->x + wall->width));
        float closestY = fmaxf(wall->y, fminf(game.joints[jointIndex].y, wall->y + wall->height));
        float deltaX = game.joints[jointIndex].x - closestX;
        float deltaY = game.joints[jointIndex].y - closestY;
        float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);
        if (distance >= collisionWidth) {
            continue;
        }

        if (distance <= NORMAL_EPSILON) {
            if (wall->width < wall->height) {
                float wallCenter = wall->x + wall->width / 2.0f;
                float normalX = game.joints[jointIndex].x < wallCenter ? -1.0f : 1.0f;
                float edgeX = normalX < 0.0f ? wall->x : wall->x + wall->width;
                game.joints[jointIndex].x = edgeX + normalX * collisionWidth;
            } else {
                float wallCenter = wall->y + wall->height / 2.0f;
                float normalY = game.joints[jointIndex].y < wallCenter ? -1.0f : 1.0f;
                float edgeY = normalY < 0.0f ? wall->y : wall->y + wall->height;
                game.joints[jointIndex].y = edgeY + normalY * collisionWidth;
            }
            continue;
        }

        float normalX = deltaX / distance;
        float normalY = deltaY / distance;
        float pushDistance = (collisionWidth - distance) * 0.5f;
        game.joints[jointIndex].x += normalX * pushDistance;
        game.joints[jointIndex].y += normalY * pushDistance;
        game.joints[jointIndex - 1].x += normalX * pushDistance * 0.5f;
        game.joints[jointIndex - 1].y += normalY * pushDistance * 0.5f;

        for (int offset = 1; offset < 4 && jointIndex + offset < game.jointCount; offset++) {
            float fade = (4 - offset) * 0.5f;
            game.joints[jointIndex + offset].x += normalX * pushDistance * fade;
            game.joints[jointIndex + offset].y += normalY * pushDistance * fade;
        }
    }
}

#define MAX_LEVEL_INFO_SIZE 64
#define MAX_SCORE_DETAILS_SIZE 512

static void updateScores(void) {
    int levelScore = (game.apples * MAP_SIZE * 2) - (game.jointCount - 20);
    float currentTime = game.ticks / (float)TARGET_FPS;
    int timePenalty = (int)(currentTime * 2.0f);
    game.currentLevelScore = levelScore - timePenalty;
    game.levelScores[getCurrentLevel() - 1] = game.currentLevelScore;

    game.totalScore = 0;
    for (int i = 0; i < MAX_LEVEL; i++) {
        game.totalScore += game.levelScores[i];
    }
    game.score = game.totalScore;

    float currentLevelTime = currentTime - game.currentLevelStartTime;
    game.levelTimes[getCurrentLevel() - 1] = currentLevelTime;

    char levelDetails[MAX_SCORE_DETAILS_SIZE] = "";
    int totalApples = buildLevelDetails(levelDetails, sizeof(levelDetails));

    riv->outcard_len =
        riv_snprintf((char *)riv->outcard,
                     RIV_SIZE_OUTCARD,
                     "JSON{\"score\":%d,\"a\":%d,\"l\":%d,\"t\":%.1f,\"c\":%d,\"lvl\":{%s}}",
                     game.score,
                     totalApples,
                     game.jointCount,
                     currentTime,
                     getCurrentLevel(),
                     levelDetails);
}

static int buildLevelDetails(char *details, size_t detailsSize) {
    int totalApples = 0;
    int currentLevel = getCurrentLevel();

    for (int i = 0; i < MAX_LEVEL; i++) {
        int level = i + 1;
        LevelConfig config = getLevelConfig(level);
        int levelApples = 0;
        if (level == currentLevel) {
            levelApples = game.apples;
        } else if (level < currentLevel) {
            levelApples = config.requiredApples;
        }
        totalApples += levelApples;

        int completed = 0;
        if (level < currentLevel ||
            (level == currentLevel && game.apples >= config.requiredApples)) {
            completed = 1;
        }

        char levelInfo[MAX_LEVEL_INFO_SIZE] = "";
        snprintf(levelInfo,
                 sizeof(levelInfo),
                 "%s\"l%d\":[%d,%d,%d,%.1f]",
                 i > 0 ? "," : "",
                 level,
                 levelApples,
                 completed,
                 game.levelScores[i],
                 game.levelTimes[i]);

        if (strlen(details) + strlen(levelInfo) < detailsSize) {
            strcat(details, levelInfo);
        }
    }

    return totalApples;
}

void gameInitialize(void) {
    memset(&game, 0, sizeof(game));
    objectsInitialize();
    cameraInitialize();
    resetLevelProgress();

    game.state = GAME_STATE_CHAR_SELECT;
    game.started = false;
    game.ended = false;
    game.moveTimer = 0;
    game.moveDelay = MOVE_DELAY_DEFAULT;
    game.growthRate = GROWTH_RATE;

    riv->outcard_len = riv_snprintf((char *)riv->outcard,
                                    RIV_SIZE_OUTCARD,
                                    "JSON{\"score\":%d,\"apples\":%d,\"length\":%d,\"time\":%.1f}",
                                    game.score,
                                    game.apples,
                                    20,
                                    0.0f);

    initializeSnakeAnimation(&game.snakeAnimation);
    initializeSkinSelector();
    startBackgroundMusic();
}

void gameStart(void) {
    if (DEBUG_MODE) {
        riv_printf("Game started\n");
    }
    game.started = true;
    game.ended = false;
    game.state = GAME_STATE_PLAYING;
    game.ticks = 0;
    game.moveTimer = 0;

    object.applePhysics.active = false;
    object.applePhysics.vx = 0.0f;
    object.applePhysics.vy = 0.0f;
    object.friction = PHYSICS_FRICTION;

    memset(game.snakeBody, 0, sizeof(game.snakeBody));
    game.headDirection = (riv_vec2i){0, -1};
    game.headPosition = (riv_vec2i){MAP_SIZE / 4, MAP_SIZE / 2};
    game.tailPosition = (riv_vec2i){game.headPosition.x, game.headPosition.y + 19};
    game.snakeBody[game.headPosition.y][game.headPosition.x] = game.headDirection;

    for (int i = 0; i < 20; i++) {
        game.snakeBody[game.headPosition.y + i][game.headPosition.x] = game.headDirection;
    }

    game.jointCount = 20;
    for (int i = 0; i < game.jointCount; i++) {
        game.joints[i].x = game.headPosition.x * TILE_SIZE + TILE_SIZE / 2;
        game.joints[i].y = (game.headPosition.y + i) * TILE_SIZE + TILE_SIZE / 2;
        game.joints[i].angle = 0.0f;
    }

    resetLevelProgress();
    cameraInitialize();
    initializeLevel(1);
    playStartSound();
}

static bool handleDebugLevelSkip(void) {
    if (!DEBUG_MODE || !riv->keys[RIV_GAMEPAD_R3].press) {
        return true;
    }

    LevelConfig config = getLevelConfig(getCurrentLevel());
    game.score += config.bonusPoints;

    int nextLevel = getCurrentLevel() + 1;
    if (nextLevel <= MAX_LEVEL) {
        initializeLevel(nextLevel);
        riv_printf("Debug: Skipped to level %d\n", nextLevel);
        return true;
    }

    gameComplete();
    playEndSound();
    riv->quit_frame = riv->frame + 5 * riv->target_fps;
    return false;
}

static void readDirectionInput(void) {
    if (riv->keys[RIV_GAMEPAD_UP].press && game.headDirection.y != 1) {
        game.headDirection = (riv_vec2i){0, -1};
    } else if (riv->keys[RIV_GAMEPAD_DOWN].press && game.headDirection.y != -1) {
        game.headDirection = (riv_vec2i){0, 1};
    } else if (riv->keys[RIV_GAMEPAD_LEFT].press && game.headDirection.x != 1) {
        game.headDirection = (riv_vec2i){-1, 0};
    } else if (riv->keys[RIV_GAMEPAD_RIGHT].press && game.headDirection.x != -1) {
        game.headDirection = (riv_vec2i){1, 0};
    }
}

static bool headTouchesApple(void) {
    float deltaX = game.headPosition.x * TILE_SIZE + TILE_SIZE / 2.0f - object.applePhysics.x;
    float deltaY = game.headPosition.y * TILE_SIZE + TILE_SIZE / 2.0f - object.applePhysics.y;
    float distanceSquared = deltaX * deltaX + deltaY * deltaY;
    float headRadius = getCurrentSkin() == SKIN_SNAKE ? SNAKE_HEAD_RADIUS : CATERPILLAR_HEAD_RADIUS;
    float collectDistance = headRadius + 7.0f;
    return distanceSquared < collectDistance * collectDistance;
}

static void growSnake(void) {
    if (game.jointCount >= MAX_JOINTS) {
        return;
    }

    int previousCount = game.jointCount;
    int nextCount = game.jointCount + game.growthRate;
    if (nextCount > MAX_JOINTS) {
        nextCount = MAX_JOINTS;
    }

    for (int i = previousCount; i < nextCount; i++) {
        game.joints[i] = game.joints[previousCount - 1];
    }
    game.jointCount = nextCount;
}

static bool moveTail(void) {
    if (game.tailPosition.x < 0 || game.tailPosition.x >= WORLD_TILES_X ||
        game.tailPosition.y < 0 || game.tailPosition.y >= WORLD_TILES_Y) {
        gameEnd();
        return false;
    }

    riv_vec2i tailDirection = game.snakeBody[game.tailPosition.y][game.tailPosition.x];
    game.snakeBody[game.tailPosition.y][game.tailPosition.x] = (riv_vec2i){0, 0};
    game.tailPosition =
        (riv_vec2i){game.tailPosition.x + tailDirection.x, game.tailPosition.y + tailDirection.y};
    return true;
}

static bool moveSnake(void) {
    game.moveTimer++;
    if (game.moveTimer < game.moveDelay) {
        return true;
    }
    game.moveTimer = 0;

    riv_vec2i nextHeadPosition = {game.headPosition.x + game.headDirection.x,
                                  game.headPosition.y + game.headDirection.y};
    if (checkCollision(nextHeadPosition)) {
        gameEnd();
        return false;
    }

    game.snakeBody[game.headPosition.y][game.headPosition.x] = game.headDirection;
    game.headPosition = nextHeadPosition;
    handleLevelTransition();

    if (headTouchesApple()) {
        game.apples++;
        growSnake();
        playEatSound();
        checkDoorState();
        if (!spawnApple()) {
            gameEnd();
            return false;
        }
    } else if (!moveTail()) {
        return false;
    }

    game.appleRotation += 0.1f;
    if (game.appleRotation > 2.0f * PI) {
        game.appleRotation -= 2.0f * PI;
    }
    return true;
}

static void pushAppleFromBody(void) {
    if (object.applePhysics.active) {
        return;
    }

    const float sideOffsets[] = {-PI / 2.0f, PI / 2.0f};
    for (int i = 2; i < game.jointCount - 1; i++) {
        float bodyRadius = getSnakeBodyWidth(i) * 0.67f;
        for (int side = 0; side < 2; side++) {
            float skinX = 0.0f;
            float skinY = 0.0f;
            getOffsetPosition(game.joints, i, sideOffsets[side], &skinX, &skinY);

            float deltaX = object.applePhysics.x - skinX;
            float deltaY = object.applePhysics.y - skinY;
            float distanceSquared = deltaX * deltaX + deltaY * deltaY;
            float collisionRadius = bodyRadius + 7.0f;
            if (distanceSquared >= collisionRadius * collisionRadius) {
                continue;
            }

            float normalX = 0.0f;
            float normalY = 0.0f;
            if (distanceSquared > NORMAL_EPSILON * NORMAL_EPSILON) {
                float distance = sqrtf(distanceSquared);
                normalX = deltaX / distance;
                normalY = deltaY / distance;
            } else {
                normalX = cosf(game.joints[i].angle + sideOffsets[side]);
                normalY = sinf(game.joints[i].angle + sideOffsets[side]);
            }

            object.applePhysics.vx = normalX * COLLISION_FORCE;
            object.applePhysics.vy = normalY * COLLISION_FORCE;
            object.applePhysics.active = true;

            LevelConfig config = getLevelConfig(getCurrentLevel());
            if (game.apples == config.requiredApples - 1) {
                playCoinBounceSound();
            } else {
                playAppleBounceSound();
            }
            return;
        }
    }
}

void gameUpdate(void) {
    if (game.state == GAME_STATE_CHAR_SELECT) {
        updateSkinSelection();
        if (isSkinSelected()) {
            gameStart();
        }
        return;
    }

    if (!game.started || game.ended || game.state == GAME_STATE_COMPLETED) {
        return;
    }

    game.ticks++;

    if (!handleDebugLevelSkip()) {
        return;
    }

    if (getCurrentSkin() == SKIN_SNAKE) {
        updateSnakeAnimation(&game.snakeAnimation);
    }
    readDirectionInput();
    if (!moveSnake()) {
        return;
    }

    cameraUpdate();
    updateJoints();
    updateScores();
    updateApplePhysics();
    pushAppleFromBody();
}

static void drawHud(void) {
    char scoreText[32] = "";
    snprintf(scoreText, sizeof(scoreText), "SCORE: %d", game.score);
    riv_draw_text(scoreText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  SCREEN_WIDTH - WALL_THICKNESS - 80,
                  WALL_THICKNESS + 5,
                  NORMAL_SCALE,
                  RIV_COLOR_LIGHTGREEN);

    char levelText[32] = "";
    snprintf(levelText, sizeof(levelText), "LEVEL %d", getCurrentLevel());
    riv_draw_text(levelText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  WALL_THICKNESS + 5,
                  WALL_THICKNESS + 5,
                  NORMAL_SCALE,
                  RIV_COLOR_LIGHTGREEN);
}

static void drawCharacter(void) {
    if (getCurrentSkin() == SKIN_SNAKE) {
        drawSnakeBody(game.joints, game.jointCount);
        drawSnakeHead(game.joints[0].x, game.joints[0].y, game.joints[0].angle);
        drawSnakeTongue(game.joints[0].x,
                        game.joints[0].y,
                        game.joints[0].angle,
                        game.snakeAnimation.tongueExtension);
    } else {
        drawCaterpillarBody(game.joints, game.jointCount, 1.0f);
        drawCaterpillarHead(
            game.joints[0].x, game.joints[0].y, game.joints[0].angle, CATERPILLAR_HEAD_RADIUS);
    }
}

static void drawDebugGeometry(void) {
    for (int i = 2; i < game.jointCount - 1; i++) {
        riv_draw_circle_fill(game.joints[i].x, game.joints[i].y, 2, RIV_COLOR_PINK);
        uint32_t transparentPink = (RIV_COLOR_PINK & 0x00FFFFFF) | 0x40000000;
        riv_draw_circle_fill(game.joints[i].x, game.joints[i].y, TILE_SIZE / 2, transparentPink);
    }

    for (int i = 0; i < game.jointCount; i++) {
        float width = getSnakeBodyWidth(i);
        float angle = game.joints[i].angle;
        float rightX = game.joints[i].x + cosf(angle + PI / 2.0f) * width;
        float rightY = game.joints[i].y + sinf(angle + PI / 2.0f) * width;
        float leftX = game.joints[i].x + cosf(angle - PI / 2.0f) * width;
        float leftY = game.joints[i].y + sinf(angle - PI / 2.0f) * width;
        riv_draw_circle_fill((int)rightX, (int)rightY, 2, RIV_COLOR_BLUE);
        riv_draw_circle_fill((int)leftX, (int)leftY, 2, RIV_COLOR_BLUE);
    }

    float collisionRadius =
        getCurrentSkin() == SKIN_SNAKE ? SNAKE_HEAD_RADIUS : CATERPILLAR_HEAD_RADIUS;
    uint32_t transparentOrange = (RIV_COLOR_ORANGE & 0x00FFFFFF) | 0x40000000;
    riv_draw_circle_fill(game.joints[0].x, game.joints[0].y, collisionRadius, transparentOrange);
}

void gameDraw(void) {
    riv_clear(RIV_COLOR_DARKSLATE);
    drawWalls();

    if (game.state == GAME_STATE_CHAR_SELECT) {
        drawSkinSelectionMenu();
        return;
    }

    drawHud();
    drawApple(game.appleRotation);
    drawCharacter();
    if (DEBUG_MODE) {
        drawDebugGeometry();
    }

    if (game.state == GAME_STATE_COMPLETED) {
        drawGameCompleted();
    } else if (game.state == GAME_STATE_OVER) {
        drawGameOver();
    }
}

static void drawGameOver(void) {
    // Semi-transparent overlay
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if ((x + y) % 2 == 0) {
                riv_draw_point(x, y, RIV_COLOR_BLACK);
            }
        }
    }

    // Draw "GAME OVER" text at top third
    riv_draw_text("GAME OVER",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  TOP_THIRD,
                  TITLE_SCALE,
                  RIV_COLOR_RED);

    // Draw score at middle
    char scoreText[32] = "";
    snprintf(scoreText, sizeof(scoreText), "SCORE: %d", game.score);
    riv_draw_text(scoreText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  MIDDLE,
                  NORMAL_SCALE,
                  RIV_COLOR_LIGHTRED);
}

static void drawGameCompleted(void) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if ((x + y) % 2 == 0) {
                riv_draw_point(x, y, (RIV_COLOR_DARKSLATE & 0x00FFFFFF) | 0x80000000);
            }
        }
    }

    // Draw congratulations text
    const char *congrats = "CONGRATULATIONS!";
    const char *complete = "ALL LEVELS COMPLETE";
    const char *scoreText = "FINAL SCORE:";
    char scoreValue[32] = "";
    snprintf(scoreValue, sizeof(scoreValue), "%d", game.score);

    int yPosition = 100;
    riv_draw_text(congrats,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  yPosition,
                  TITLE_SCALE,
                  RIV_COLOR_GOLD);

    yPosition += 30;
    riv_draw_text(complete,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  yPosition,
                  1.5f,
                  RIV_COLOR_LIGHTGREEN);

    yPosition += 30;
    riv_draw_text(scoreText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  yPosition,
                  NORMAL_SCALE,
                  RIV_COLOR_WHITE);

    yPosition += 15;
    riv_draw_text(scoreValue,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  yPosition,
                  1.5f,
                  RIV_COLOR_GOLD);
}

void gameEnd(void) {
    if (DEBUG_MODE) {
        riv_printf("Game Over triggered at frame %d\n", riv->frame);
        riv_printf(
            "Final score: %d, Apples: %d, Length: %d\n", game.score, game.apples, game.jointCount);
    }
    game.ended = true;
    game.state = GAME_STATE_OVER;
    riv->quit_frame = riv->frame + 3 * riv->target_fps;
    stopBackgroundMusic();
    playEndSound();
}

void gameComplete(void) {
    if (DEBUG_MODE) {
        riv_printf("Game Completed at frame %d\n", riv->frame);
        riv_printf("Final score: %d, Total apples: %d\n", game.score, game.apples);
    }

    game.ended = true;
    game.state = GAME_STATE_COMPLETED;
    riv->quit_frame = riv->frame + 5 * riv->target_fps;

    stopBackgroundMusic();
    playVictoryFanfare();
}

int main(void) {
    riv->width = SCREEN_WIDTH;
    riv->height = SCREEN_HEIGHT;
    riv->target_fps = TARGET_FPS;

    audioInitialize();
    gameInitialize();

    while (riv_present()) {
        gameUpdate();
        gameDraw();
        playBackgroundMusic();
    }

    return 0;
}
