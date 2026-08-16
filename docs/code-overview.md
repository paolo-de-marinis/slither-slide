# Slither Slide code overview

Slither Slide is written as procedural C with separate modules. The program keeps the frame order visible and passes explicit state to the functions that change it.

## Start here

The entry point in `main.c` is short:

~~~c
while (riv_present()) {
    developerControlsUpdate(&developerControls,
                            riv->keys[CONTROL_TECHNICAL_VIEW].press);
    gameUpdate();
    gameDraw(&developerControls);
    playBackgroundMusic();
}
~~~

This order matters. Input is read from the RIVES key state, gameplay advances once, the resulting state is drawn, and the sequenced music is polled last.

The main gameplay state is the global `GameData game` declared in `game.c` and described in `game_state.h`. It contains:

- logical head, tail and direction data on the global snake lattice;
- the world-space floating-point joint chain used for animation and drawing;
- movement timers and animation state;
- per-level items, completion flags, elapsed time and score;
- the current lifecycle state.

The four lifecycle states are character selection, active play, game over and completion. `gameUpdate()` handles character selection first, ignores terminal states, and then performs the active-play update in this order:

1. optional cheat handling;
2. snake animation;
3. logical movement and collection;
4. camera update;
5. joint-chain constraints;
6. score and time update;
7. collectible physics.

An early return stops the remainder of the frame when a collision, completion or initialization failure changes the game state.

## Spatial model

The shortest accurate description is: one global world, but no single global tile map.
`LEVEL_MATRIX` places complete 256 × 256 rooms. The separate global snake lattice stores the
logical head, tail and direction field. Mapping a snake cell $q=(q_x,q_y)$ to
$\Phi(q)=(6q_x+3,6q_y+3)$ then supplies the world point used by geometric collision.

Walls and the continuous joint chain already live in that world. A collectible is different
only during placement: `collectibleSpawn()` selects a room-local integer candidate and
immediately converts it to floating-point world position and velocity. Drawing later projects
the world through the camera and constructs the B-spline in screen coordinates.

The distinction matters because $256=42\cdot6+4$: room boundaries, the global snake lattice
and room-local spawn candidates need not align. Door crossing and collection work by geometric
tests, not by asking whether every object occupies the same cell. The complete derivation is
in [How Slither Slide represents space](spatial-model.md).

## Logical movement

`snake_motion.c` owns the tile-grid part of the snake:

- `readDirectionInput()` rejects an immediate reversal;
- `moveTail()` follows `bodyDirections` and clears the old tail cell;
- `snakeMotionUpdate()` checks the next head position, moves the head and processes collection;
- `snakeCollectItem()` updates room progress and grows the body.

`bodyDirections[y][x]` stores the direction that the tail must follow from an occupied path
cell. It is not a room tile map: walls and collectibles are never stored in it. A proposed
integer head cell is mapped to the world-space center
$\Phi(q_x,q_y)=(6q_x+3,6q_y+3)$ before geometric collision tests. The joint chain is a
separate continuous layer and never replaces the logical position used by the movement rules.

## Rooms, walls and doors

The twelve levels occupy a 4 × 3 matrix in `room_layout.c`. Each entry represents a complete
256 × 256 region of one 1024 × 768 world. `levels.c` stores level requirements and rebuilds the
active boundary and obstacle rectangles in global coordinates for the current room.

The outer boundary is divided into eight segments per side. Opening a door means removing the
two central segments on the traversable side. `canTraverseLevels()` permits forward movement
when the levels are adjacent and the room being left is complete; backtracking to the previous
level remains available.

The head advances in global 6-pixel steps, while a room is 256 pixels wide. Since 256 is not a
multiple of 6, room edges are geometric boundaries rather than grid lines. After an allowed
crossing, `handleLevelTransition()` classifies the new world-space head center with integer
division by the room dimensions and reads the new level from `LEVEL_MATRIX`.

`collision.c` maps a proposed head cell to world coordinates and checks it against:

- world and room boundaries;
- the wall rectangles currently stored by `walls.c`;
- sampled segments of the body chain.

The head radius is the original skin-independent value, `8.0f * 0.67f`. The level-2 square obstacle is tested against the same inset rectangle that is drawn.

## From joints to the visible snake

The geometry path is split into three steps.

### 1. Joint constraints

`body_chain.c` pins the first world-space joint to the center mapped from the logical head, lets
each following joint approach the previous one, separates distant non-neighbouring joints when
they overlap and pushes the chain out of walls. The head is pinned again before the final
tangent pass, so every stored angle is computed from the corrected positions.

### 2. B-spline samples

`snake_char.c` first projects the joints from world to screen coordinates. `snake_geometry.c`
then builds the outline controls from left and right offsets around those projected joints and
evaluates every periodic cubic span at two parameters. `spline_math.c` contains only the four
basis weights.

The resulting `SnakeGeometry` contains:

- the complete closed control polygon;
- the two samples per span consumed by the renderer.

### 3. Drawing

`snake_char.c` pairs samples from the two sides and fills each strip with two triangles. The head, tongue and scale marks are added afterwards. The geometry buffers are static because they are reused every frame and are too large to allocate repeatedly on the stack.

The caterpillar follows the same joint chain but has its own simpler renderer in `caterpillar_char.c`.

## Technical view

`developer_controls.c` stores only the toggle and active-span selection. This state is deliberately separate from `GameData` because it belongs to the explanatory interface rather than to the run.

`technical_view.c` receives const pointers to the live joints and `SnakeGeometry`. It draws the chain, all controls, the selected four-control window, renderer samples and the moving point $C_i(t)$. It cannot change movement, collision, score, timers, RNG or outcard data.

The selected span advances automatically until the first A/F or L2/R2 press. Manual navigation then moves the window by one control point and wraps around the current polygon.

## Collectibles and score

`collectible.c` keeps the current apple or coin, handles its short bounce response and finds a
legal spawn position. It enumerates room-local grid candidates, filters walls, the HUD,
corners, the existing continuous joint chain and the extra coin margin, then converts the
selected cell to a floating-point world position. Once pushed, the item moves with velocity
and friction and is not snapped back to that spawn cell. The scan order is fixed because
changing it would also change deterministic RIVES entropy consumption.

`scoring.c` records active time by level. A completed room keeps its result, so revisiting it does not restart its timer or create another item. The JSON outcard contains the total score, collected items, body length, total time and compact per-level data.

## Rendering and audio

`camera.c` smoothly approaches the current room origin and exposes the translation
`worldToScreen()`. Collision and physics stay in world coordinates. `game_render.c` draws the
projected world before the HUD and character. The Technical view is drawn only for the snake
skin. Game-over and completion panels are terminal overlays.

`audio.c` is the only translation unit that defines `SEQT_IMPL`. It owns the background sequencer and exposes small functions for the sound effects used by the gameplay modules.

## Build flags

`DEBUG_MODE` and `CHEATS_ENABLED` are independent and default to zero:

- debug mode adds diagnostics;
- cheats enable R/R3 level advancement through the ordinary completion path;
- the Technical view is available in the normal build and does not depend on either flag.

## Tests

The host checks in `tests/` cover the room matrix and doors, scoring, body-chain invariants, B-spline basis and periodic geometry, Technical-view controls, completion events, spawn failure and collision clearance. They use the same production functions where possible and small RIVES stubs where the platform context is required.
