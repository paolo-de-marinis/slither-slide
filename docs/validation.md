# Validation

## Scope

Validation is separated into host-side invariants and RIVES integration. The first group can run with a standard C11 compiler and is executed by `make -C src test`. Packaging and a complete playthrough require RIVEMU, the RIV OS SDK and the cartridge's audio asset.

## 1. Strict host compilation

Every maintained translation unit was compiled independently in both the default and `DEBUG_MODE=1` configurations with

```text
-std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror -fanalyzer
```

Run the same check with `make -C src strict`. It checks declarations, module interfaces, type usage and warning cleanliness without replacing the RISC-V build.

## 2. Automated host checks

Run

```sh
make -C src test
```

The target builds and executes six focused programs:

| Test | Properties checked |
|---|---|
| `test_room_layout.c` | room coordinates, adjacency and directional traversal rule |
| `test_level_doors.c` | all eleven consecutive transitions, both sides of each door, invalid configuration lookup and completed-room backtracking |
| `test_scoring.c` | historical pre-start outcard, per-level time penalty, persistent completion bonus, frozen completed score and paused/resumed incomplete clock |
| `test_body_chain.c` | final coincidence of logical head and joint zero, and tangent reconstruction from final positions |
| `test_spline_math.c` | non-negative cubic basis, partition of unity at 101 samples, exact endpoint weights and the midpoint tuple $(1,23,23,1)/48$ |
| `test_gameplay_events.c` | normal completion after the level-12 coin, absence of a post-completion spawn and explicit spawn-failure termination |

The current host results are:

```text
room layout: ok
level doors and backtracking: ok
per-level scoring: ok
body-chain invariants: ok
B-spline basis invariants: ok
gameplay completion and spawn failure: ok
```

## 3. Behavior changes and their evidence

The maintained source intentionally differs from the earlier cleaned source in the following cases. The automated suites directly cover final completion, spawn failure, score persistence, per-level timing, backtracking, the final head anchor and tangent reconstruction:

- collecting the required coin completes a room and does not generate another collectible;
- collecting the level-12 coin calls `gameComplete()` through the normal, non-debug path;
- the configured bonus is added when the room is completed and remains in its frozen level score;
- the time penalty uses active time in that level rather than total run time;
- leaving an incomplete room pauses its clock, while revisiting a completed room does not change its result or call the spawn routine;
- a failed required spawn terminates the run instead of leaving the room without an objective;
- joint zero is re-anchored after the constraint pass and all tangents are then reconstructed.

Source inspection additionally confirms that head collision, collection distance and debug display all call `collisionHeadRadius()`, which selects the active skin's explicit radius. It also confirms that the row-major candidate scan inside `collectibleSpawn()` and its single random-index selection remain in the same order. The current suite does not invoke that concrete spawn implementation, so its candidate list and RIVES entropy call count still require an integration baseline. The deliberate removal of post-completion and completed-room spawns removes random calls that no longer correspond to an active objective.

## 4. RIVES integration boundary

The source layout keeps `SEQT_IMPL` in `audio.c` only, compiles every module separately and links the explicit object list. The expected integration commands are

```sh
make -C src clean all
make -C src smoke
```

The source before this module-and-behavior refactor had been packaged with RIVEMU 0.3.0 and RIV OS SDK `v0.3-rc16` and had completed a 180-frame headless smoke run. Those earlier screenshot and outcard hashes are not claimed for the maintained refactor: score semantics, final completion and module boundaries changed intentionally. A fresh RISC-V package build and deterministic capture must therefore be performed in an environment containing the SDK and `songs/gameplay.rivcard` before recording new integration hashes.

## 5. Remaining validation limits

- Host tests use controlled stubs around the RIVES drawing and audio calls required by the isolated modules; the current Slither Slide suite does not exercise the concrete entropy path.
- They verify the basis identities numerically at a finite sample set; the derivation in the mathematics document supplies the general argument.
- Collision remains an intentional approximation of the rendered spline surface.
- A full twelve-room human playthrough is still required to assess difficulty, visual transitions and audio timing.
