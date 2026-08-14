# Validation

The host checks cover rules and geometry with an ordinary C11 compiler. The repository does not vendor the RIV API header: `strict` and `test` obtain `riv.h` from the installed official RIV SDK and copy it only into a temporary build directory. RIVEMU and the RIV OS SDK are therefore required both for those checks and for the real RISC-V build, packaging and runtime checks.

## Host compilation

~~~sh
make -C src strict
~~~

Every production source file is compiled separately in three configurations:

- normal release;
- `DEBUG_MODE=1`;
- `CHEATS_ENABLED=1`.

The check uses:

~~~text
-std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror -fanalyzer
~~~

This catches mismatched declarations, unused configuration-specific code and warning-level defects before the RIVES build.

## Automated tests

~~~sh
make -C src test
~~~

| Test | Main checks |
| --- | --- |
| `test_room_layout.c` | room coordinates and adjacency |
| `test_level_doors.c` | all eleven doors, both directions and backtracking |
| `test_scoring.c` | per-level clocks, bonuses and frozen completed scores |
| `test_body_chain.c` | final head anchor and reconstructed tangents |
| `test_spline_math.c` | non-negativity, partition of unity and exact basis values |
| `test_snake_geometry.c` | control count, periodic closure and stored samples |
| `test_developer_controls.c` | toggle, automatic selection, manual steps and wrapping |
| `test_gameplay_events.c` | final completion and spawn-failure handling |
| `test_collision.c` | original head radius and obstacle clearance |

Expected output:

~~~text
room layout: ok
level doors and backtracking: ok
per-level scoring: ok
body-chain invariants: ok
B-spline basis invariants: ok
snake geometry: ok
developer controls: ok
gameplay completion and spawn failure: ok
collision radius, level-2 cube and level-5 clearance: ok
~~~

These tests cover several corrections made during maintenance:

- the level-12 coin completes the run through the normal gameplay path;
- completed rooms do not spawn another item or restart their clock;
- a required spawn failure ends the run instead of leaving an unwinnable room;
- the head is pinned again after the constraint pass;
- tangents are computed from the final joint positions;
- collision uses the original skin-independent radius $8\cdot0.67=5.36$ pixels;
- the level-2 square is tested against the same inset rectangle that is drawn;
- Technical-view state remains separate from gameplay state.

## RIVES build

~~~sh
make -C src clean all
make -C src smoke
~~~

The latest recorded normal build used RIVEMU/libriv 0.3.0 and RIV OS SDK 0.3.0. Each module was compiled with `riv-opt-flags -Ospeed`, the executable was processed by `riv-strip`, and the packaged cartridge completed a 180-frame headless run at the official 96 MB runtime limit.

The resulting `snake.sqfs` measured 28,672 bytes, below the current 262,144-byte RIVES upload limit.

## Visual check

The images in `docs/media/` were captured from the normal build with debug and cheats disabled. The recorded run checked:

- character selection and start;
- direction changes;
- D/R1 Technical-view toggle;
- A/F and L2/R2 one-span navigation;
- continued movement while the overlay was visible.

The same cartridge was opened in the official web emulator, where the selection screen, start, movement and Technical-view controls responded as expected.

## What is not covered

- The host suite stubs RIVES drawing and audio calls; it does not compare rendered pixels or sound.
- The concrete collectible entropy path is not replayed by the current host tests.
- Collision approximates the joint chain rather than the filled B-spline surface.
- Difficulty, room-to-room pacing and audio timing still require a complete human playthrough.
