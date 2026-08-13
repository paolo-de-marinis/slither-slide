# Slither Slide code overview

## Scope

This document describes the maintained source of the Slither Slide RIVES cartridge. The program is procedural C: state is represented explicitly, modules group related operations and the frame loop makes the update order visible. The decomposition is intended to remain approachable to a reader who knows structures, arrays, functions and separate compilation.

## 1. Module boundaries

The source is divided by responsibility rather than by individual function:

| Module | Responsibility |
|---|---|
| `main.c` | RIVES dimensions, input tracking, target frame rate and frame loop |
| `controls.h` | canonical Technical-view and cheat-button mapping |
| `developer_controls.c` | app-level runtime state for the Technical-view toggle and span selection |
| `game_state.h` | `GameData`, global constants and the single state declaration |
| `game.c` | initialization, frame orchestration, optional cheat progression and terminal states |
| `snake_motion.c` | D-pad input, logical head/tail movement, growth and collection events |
| `collision.c` | head radius and collision predicates |
| `body_chain.c` | position-based joint constraints and tangent reconstruction |
| `scoring.c` | per-level time, persistent scores and JSON outcard |
| `game_render.c` | HUD, character selection, Technical-view dispatch and ending panels |
| `levels.c` | level table, completion state, doors, transitions and obstacle declarations |
| `room_layout.c` | the 4 × 3 room matrix and adjacency relation |
| `collectible.c` | apple/coin state, spawn and short physics response |
| `collectible_render.c` | apple and coin primitive rendering |
| `walls.c` | wall storage, circle/rectangle contacts and wall rendering |
| `snake_geometry.c` | pure profile construction, periodic B-spline evaluation and sampling |
| `snake_char.c` | sampled-geometry rasterization, scales, head and tongue |
| `spline_math.c` | cubic uniform B-spline basis |
| `technical_view.c` | read-only visualization of the spline construction and parametrization |
| `caterpillar_char.c` | alternate character renderer |
| `camera.c` | world-to-screen transformation and eased room transitions |
| `char_selector.c` | initial skin menu |
| `audio.c` | effects and the only `SEQT_IMPL` translation unit |

The decomposition keeps orchestration, geometry, scoring, stateful object logic and rendering separate while leaving the already focused character, camera, audio and room-layout modules intact.

`riv.h` is the preserved RIVES API header; the corresponding upstream file is the official [RIV API header](https://github.com/rives-io/riv/blob/main/libriv/riv.h). `seqt.h` supplies the sequenced-audio implementation. The Makefile compiles every `.c` file separately and links the resulting object files explicitly.

## 2. State and invariants

`GameData` in `game_state.h` owns the state that must persist across modules. Its main groups are:

- `state` for the four lifecycle states;
- `headPosition`, `headDirection`, `tailPosition` and `bodyDirections` for the logical grid snake;
- `joints` and `jointCount` for the continuous-looking body;
- `moveTimer`, `moveDelay` and `growthRate` for level-dependent movement;
- `levelItems`, `levelScores`, `levelElapsedTicks` and `levelCompleted` for persistent progress;
- `snakeAnimation` and `collectibleRotation` for visual animation.

The implementation maintains the following invariants:

1. `headPosition` and `tailPosition` are world-grid coordinates, whereas joint coordinates are world-space pixels.
2. `bodyDirections[y][x]` stores the direction that the tail must follow when it reaches an occupied tile.
3. After `bodyChainUpdate()`, `joints[0]` coincides exactly with the center of the logical head tile.
4. Joint angles are recomputed from the final corrected chain, not from an intermediate solver state.
5. A completed level has a final score, a frozen elapsed time and no collectible.
6. A completion bonus is incorporated exactly once into that level's stored score.

The single global `game` object is declared in `game_state.h` and defined in `game.c`. Computational modules receive a `GameData *` or `const GameData *` when operating on it; this keeps data flow explicit even though the cartridge has only one game instance. `DeveloperControls` is owned separately by `main()` and contains the Technical-view toggle plus its manual span-selection state. Neither `developer_controls.c` nor `technical_view.c` receives a `GameData *`, so the view cannot mutate gameplay or outcard state.

## 3. Frame flow

Execution starts in `main.c`:

1. configure the 256 × 256 output and 60 FPS target;
2. initialize audio, the game and the disabled-by-default developer-control state;
3. for each successful `riv_present()`, update the D/R1 toggle, then call `gameUpdate()`, `gameDraw()` and `playBackgroundMusic()`.

During the selection screen, `gameUpdate()` delegates to `updateSkinSelection()`. During play, one frame performs:

1. advance the global tick count;
2. process the optional cheat level skip when `CHEATS_ENABLED=1`;
3. advance tongue animation for the snake skin;
4. read input and, when the movement timer expires, move the logical snake;
5. update the camera;
6. solve the body-chain constraints and recompute tangents;
7. update the active level score and the outcard;
8. integrate collectible physics and test body interaction.

Rendering is kept in `game_render.c`. It clears the frame, draws the current walls, then draws either the selection menu or the HUD, collectible and character. When the snake skin and Technical view are active, it passes the renderer's current `SnakeGeometry` to `technicalViewDraw()`. Ending panels are overlays on the last game frame.

## 4. Logical movement and continuous geometry

The cartridge deliberately uses two body representations:

- the grid map `bodyDirections`, which gives deterministic head and tail movement;
- the floating-point joint chain, which supports a curved body, rendering and approximate physical interaction.

`snakeMotionUpdate()` rejects an immediate 180-degree reversal, waits for the level-specific delay, constructs the next head tile and asks `collisionAtNextHead()` whether the move is admissible. If no item is collected, `moveTail()` clears the old tail tile and follows its stored direction. On collection, `bodyChainGrow()` duplicates the last joint and the tail stays still for that move.

`bodyChainUpdate()` treats the logical head as a boundary condition. It first anchors joint zero, then applies follower spacing, non-neighbour overlap resolution and wall corrections to the remaining chain. It anchors joint zero again after those corrections and finally recomputes every angle from the resulting geometry. The direct overlap pass is quadratic in the joint count; current playable lengths make the simple implementation preferable to a spatial index.

The mathematical construction of the profile and spline is developed separately in [Mathematics of the procedural body](b-spline.md). `snakeGeometryBuild()` is the single construction path: both ordinary rasterization and the Technical view consume its control-point and sample arrays.

## 5. Collision geometry

`collisionAtNextHead()` combines three predicates:

- a circle against the current room boundary, with exceptions only in traversable door intervals;
- a circle against each non-empty axis-aligned wall rectangle;
- a circle against five uniformly spaced points on each sufficiently distant body segment.

`collisionHeadRadius()` returns the skin-independent historical gameplay radius `8 × 0.67 = 5.36` pixels. The same value is used for movement collision and collection distance. Keeping visual skin dimensions separate from this logical radius prevents a cosmetic choice from changing level clearance. The visible decoration may extend beyond the logical radius; the collision surface is intentionally simpler and slightly more forgiving than the rendered surface. Interior-obstacle head tests use the same one-pixel inset rectangle that `wallsDraw()` fills, so the level-2 cube cannot trigger against an invisible outer pixel. Boundary-wall handling is unchanged.

`wallCircleContact()` centralizes the closest-point calculation used by head collision, body correction, spawn exclusion and collectible physics. This avoids four separate versions of the same circle/rectangle predicate.

## 6. Levels, completion and backtracking

The room matrix is

```text
 1   2  11  10
 4   3  12   9
 5   6   7   8
```

Consecutive level numbers are adjacent in this matrix. `canTraverseLevels()` permits a forward move only after the current room is complete and always permits a move to the immediately preceding room.

`initializeLevel()` loads movement parameters, builds boundary walls and obstacles, opens currently available doors and starts or resumes that level's clock. For an incomplete level it generates one collectible. For a completed level it explicitly hides the collectible; revisiting the room therefore consumes no RIVES entropy and cannot change its result.

`checkDoorState()` is the completion transaction:

1. verify that the configured item requirement has been reached;
2. mark the room complete exactly once;
3. finalize its time and score, including the completion bonus;
4. hide the collectible;
5. open traversable doors and play the door sound.

The final required item is rendered as a coin. In rooms 1–11, collecting it completes the room without spawning another object. In room 12, the same ordinary completion path is followed and `gameComplete()` then enters the final state. A failed spawn is not ignored: it ends the run instead of leaving an invisible objective.

Backtracking preserves item counts, scores, elapsed ticks and completion flags. An incomplete level's clock is paused when the player leaves it and resumes on return; completed scores and times remain frozen. Seconds are derived from the integer tick counts when the score or outcard is produced, so there is no second mutable time representation.

## 7. Score model

For level $r$, let

- $a_r$ be the number of collected items;
- $n_r$ be the snake length when room $r$ is evaluated, frozen on completion;
- $\tau_r$ be active time in that level, in seconds;
- $B_r$ be the configured completion bonus, or zero before completion.

The stored level score is

```math
S_r = 2M a_r - (n_r-n_0) - \lfloor 2\tau_r\rfloor + B_r,
```

where $M=42$ is `MAP_SIZE` and $n_0=20$ is `INITIAL_SNAKE_LENGTH`. The total score is

```math
S=\sum_{r=1}^{12}S_r.
```

Only active time in the current incomplete room contributes to $\tau_r$. `scoringCompleteLevel()` first captures the final elapsed interval, computes the non-bonus part with that room's current length $n_r$ and then adds $B_r$ before freezing the result. `scoringUpdate()` may therefore recompute an incomplete room, but it cannot overwrite a completed room's bonus. A room not yet entered contributes zero rather than evaluating the formula with a fictitious length or time.

The outcard reports total score, total items, current length, total run time, current room and the per-room tuple `[items, completed, score, time]`.

## 8. Deterministic collectible generation

`collectibleSpawn()` enumerates candidate cells in a fixed row-major order. It excludes:

- the HUD footprint and corner buffers;
- a larger edge margin for the coin;
- wall contacts with two pixels of clearance;
- positions close to any body joint.

It then makes one `riv_rand_uint()` call to choose an index. The scan order is intentionally documented beside the loop because changing it changes the chosen cell for the same RIVES entropy. Completed-room visits make no random call.

When the body profile touches the object, the object receives a short normal impulse. Velocity is damped every frame, reflected at room bounds and wall contacts, and set to zero below a fixed threshold.

## 9. Technical view, diagnostics and cheats

The runtime Technical view is compiled into the normal cartridge, starts disabled and toggles with D/R1. It is restricted to the snake skin and shows:

- the center-chain joints;
- every lateral profile control in light blue and their closed control polygon;
- the resulting B-spline and every renderer sample at $t=0$ and $t=0.5$;
- four orange rings around only the controls $P_i,\ldots,P_{i+3}$ of the active span, its point $C_i(t)$ and the four basis weights.

Its data flow is read-only and stops at drawing calls. The animation parameter is derived from `riv->frame`; it does not consume RIVES entropy or update `GameData`. Automatic span selection continues until the first A/F or L2/R2 press. Manual navigation changes $i$ by exactly one in either direction and wraps against the live `controlPointCount`, including when growth changes that count.

`DEBUG_MODE` separately enables collision, lifecycle and music logging. `CHEATS_ENABLED` separately makes R/R3 complete the current level through `checkDoorState()` and enter the next room through `initializeLevel()`; at level 12 it calls `gameComplete()`. The cheat therefore exercises the same completion, score and door operations as normal collection, but neither compile-time flag controls the Technical view.

## 10. RIVES integration

- `riv_present()` advances the fixed-rate frame loop and input state;
- `riv->keys` exposes gamepad transitions;
- `riv_draw_*` and `riv_clear()` render primitives and text;
- `riv_rand_uint()` selects a deterministic spawn candidate;
- `riv_waveform()` produces effects and `seqt_poll()` advances music;
- `riv_snprintf()` and `riv->outcard` publish results;
- `riv->quit_frame` schedules exit after game over or completion.

## 11. Remaining limits

- Diagnostics and cheats are selected independently at build time; the Technical view is a runtime toggle.
- The rendered B-spline triangles are not the collision surface.
- `MAX_JOINTS` is a fixed upper bound substantially larger than ordinary play requires.
- The end states schedule cartridge exit and do not offer an in-process restart.
- Host tests exercise the pure and orchestration-level invariants, but they do not replace a complete twelve-room RIVEMU playthrough.
