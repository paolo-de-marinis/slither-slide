# Validation

## Scope

Validation is separated into host-side invariants and RIVES integration. The first group can run with a standard C11 compiler and is executed by `make -C src test`. Packaging and a complete playthrough require RIVEMU, the RIV OS SDK and the cartridge's audio asset.

## 1. Strict host compilation

Every maintained translation unit is compiled independently to an object in three configurations: default release, `DEBUG_MODE=1` with cheats disabled, and `CHEATS_ENABLED=1` with diagnostics disabled. All use

```text
-std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror -fanalyzer
```

Run the same check with `make -C src strict`. It checks declarations, module interfaces, type usage and warning cleanliness without replacing the RISC-V build.

## 2. Automated host checks

Run

```sh
make -C src test
```

The target builds and executes nine focused programs:

| Test | Properties checked |
|---|---|
| `test_room_layout.c` | room coordinates, adjacency and directional traversal rule |
| `test_level_doors.c` | all eleven consecutive transitions, both sides of each door, invalid configuration lookup and completed-room backtracking |
| `test_scoring.c` | historical pre-start outcard, per-level time penalty, persistent completion bonus, frozen completed score and paused/resumed incomplete clock |
| `test_body_chain.c` | final coincidence of logical head and joint zero, and tangent reconstruction from final positions |
| `test_spline_math.c` | non-negative cubic basis, partition of unity at 101 samples, exact endpoint weights and the midpoint tuple $(1,23,23,1)/48$ |
| `test_snake_geometry.c` | $2m+1$ controls, two samples per span, exact profile controls, periodic span closure, stored-sample agreement and the midpoint weighted sum |
| `test_developer_controls.c` | initial automatic selection, one-span L2/R2 steps, both wrap directions, disabled-view isolation and normalization after control-count changes |
| `test_gameplay_events.c` | normal completion after the level-12 coin, absence of a post-completion spawn and explicit spawn-failure termination |
| `test_collision.c` | historical 5.36-pixel head radius, the exact level-2 cube-corner clearance against the drawn rectangle, its adjacent fatal contact and the preserved level-5 clearance |

The current host results are:

```text
room layout: ok
level doors and backtracking: ok
per-level scoring: ok
body-chain invariants: ok
B-spline basis invariants: ok
snake geometry: ok
developer controls: ok
gameplay completion and spawn failure: ok
collision radius, level-2 cube and level-5 clearance: ok
```

## 3. Behavior changes and their evidence

The maintained source intentionally differs from the earlier cleaned source in the following cases. The automated suites directly cover final completion, spawn failure, score persistence, per-level timing, backtracking, the final head anchor, tangent reconstruction and the restored collision clearance:

- collecting the required coin completes a room and does not generate another collectible;
- collecting the level-12 coin calls `gameComplete()` through the normal, non-debug path;
- the configured bonus is added when the room is completed and remains in its frozen level score;
- the time penalty uses active time in that level rather than total run time;
- leaving an incomplete room pauses its clock, while revisiting a completed room does not change its result or call the spawn routine;
- a failed required spawn terminates the run instead of leaving the room without an objective;
- joint zero is re-anchored after the constraint pass and all tangents are then reconstructed.
- head collision and collection distance use the original skin-independent radius `8 × 0.67 = 5.36` pixels; a cosmetic skin choice therefore does not alter gameplay clearance;
- head collision against interior obstacles uses their visibly filled one-pixel-inset rectangle; the level-2 lower corner therefore keeps its real one-pixel clearance, while perimeter handling and true adjacent contact remain unchanged;
- the Technical-view toggle and manual span index remain outside `GameData`, and its draw API receives only read-only geometry plus display-only span data;
- the renderer and Technical view share the same control and sample arrays, so the visualization cannot drift into a second spline implementation;
- manual span navigation advances by one, wraps modulo the live control-point count and remains valid as growth changes that count;
- diagnostics and cheats compile independently, while D/R1 remains available in the default build.

Source inspection additionally confirms that head collision, collection distance and debug display all call `collisionHeadRadius()`. It also confirms that the row-major candidate scan inside `collectibleSpawn()` and its single random-index selection remain in the same order. The current suite does not invoke that concrete spawn implementation, so its candidate list and RIVES entropy call count still require an integration baseline. The deliberate removal of post-completion and completed-room spawns removes random calls that no longer correspond to an active objective.

## 4. RIVES integration boundary

The source layout keeps `SEQT_IMPL` in `audio.c` only, compiles every module separately and links the explicit object list, including `snake_geometry`, `developer_controls` and `technical_view`. The expected integration commands are

```sh
make -C src clean all
make -C src smoke
```

The current normal cartridge was packaged with RIVEMU/libriv 0.3.0 and RIV OS SDK 0.3.0. It completed the 180-frame headless run with the official 96 MB runtime limit; the resulting `snake.sqfs` is 32768 bytes, below the 524288-byte cartridge limit.

## 5. Runtime visual verification

The images in `docs/media/` come from the normal cartridge build with `DEBUG_MODE=0` and `CHEATS_ENABLED=0`. In a live RIVEMU session, the snake skin was selected and moved through several direction changes before D/R1 enabled the Technical view. A/F then moved the active four-control window by one span at a time while the light-blue complete control set remained visible and the white $C_i(t)$ point continued to animate. The overlay inputs did not pause or alter continuing movement.

The PNG files are frame captures from that recorded run. The GIF replays the same normal-build input log and includes both overlay transitions and continued motion. This is a visual and integration check of the displayed geometry, colors, labels and input path; it is not a new mathematical test of the spline.

The same `snake.sqfs` was also loaded in the official web emulator. The selection screen, Z start, direction input, D toggle and A/F span navigation all rendered and responded as expected there.

## 6. Remaining validation limits

- Host tests use controlled stubs around the RIVES drawing and audio calls required by the isolated modules; the current Slither Slide suite does not exercise the concrete entropy path or compare Technical-view pixels.
- They verify the basis identities numerically at a finite sample set; the derivation in the mathematics document supplies the general argument.
- Collision remains an intentional approximation of the rendered spline surface.
- A full twelve-room human playthrough is still required to assess difficulty, visual transitions and audio timing.
