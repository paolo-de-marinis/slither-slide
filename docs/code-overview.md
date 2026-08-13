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

- logical head, tail and direction data on the tile grid;
- the floating-point joint chain used for drawing;
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

## Logical movement

`snake_motion.c` owns the tile-grid part of the snake:

- `readDirectionInput()` rejects an immediate reversal;
- `moveTail()` follows `bodyDirections` and clears the old tail cell;
- `snakeMotionUpdate()` checks the next head position, moves the head and processes collection;
- `snakeCollectItem()` updates room progress and grows the body.

The grid makes movement and room transitions discrete. The joint chain is a separate visual layer and never replaces the logical position used by the rules.

## Rooms, walls and doors

The twelve levels occupy a 4 × 3 matrix in `room_layout.c`. `levels.c` stores level requirements and builds the walls and obstacles for the current room.

The outer boundary is divided into segments. Opening a door means removing the corresponding segments on the two adjacent sides. `canTraverseLevels()` permits a transition when the levels are adjacent and the room being left is complete.

`collision.c` checks a proposed head position against:

- world and room boundaries;
- the wall rectangles currently stored by `walls.c`;
- sampled segments of the body chain.

The head radius is the original skin-independent value, `8.0f * 0.67f`. The level-2 square obstacle is tested against the same inset rectangle that is drawn.

## From joints to the visible snake

The geometry path is split into three steps.

### 1. Joint constraints

`body_chain.c` pins the first joint to the logical head, lets each following joint approach the previous one, separates distant non-neighbouring joints when they overlap and pushes the chain out of walls. The head is pinned again before the final tangent pass, so every stored angle is computed from the corrected positions.

### 2. B-spline samples

`snake_geometry.c` builds the outline controls from left and right offsets around the joints. It then evaluates every periodic cubic span at two parameters. `spline_math.c` contains only the four basis weights.

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

`collectible.c` keeps the current apple or coin, handles its short bounce response and finds a legal spawn position. Spawn candidates must avoid walls, the HUD, corners, the existing body and the extra coin margin. The scan order is fixed because changing it would also change deterministic RIVES entropy consumption.

`scoring.c` records active time by level. A completed room keeps its result, so revisiting it does not restart its timer or create another item. The JSON outcard contains the total score, collected items, body length, total time and compact per-level data.

## Rendering and audio

`game_render.c` draws the world before the HUD and character. The Technical view is drawn only for the snake skin. Game-over and completion panels are terminal overlays.

`audio.c` is the only translation unit that defines `SEQT_IMPL`. It owns the background sequencer and exposes small functions for the sound effects used by the gameplay modules.

## Build flags

`DEBUG_MODE` and `CHEATS_ENABLED` are independent and default to zero:

- debug mode adds diagnostics;
- cheats enable R/R3 level advancement through the ordinary completion path;
- the Technical view is available in the normal build and does not depend on either flag.

## Tests

The host checks in `tests/` cover the room matrix and doors, scoring, body-chain invariants, B-spline basis and periodic geometry, Technical-view controls, completion events, spawn failure and collision clearance. They use the same production functions where possible and small RIVES stubs where the platform context is required.
