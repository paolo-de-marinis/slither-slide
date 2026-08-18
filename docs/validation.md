# Validation

The repository uses several kinds of evidence, and they should not be conflated.

A compiler check can establish that every production translation unit satisfies the selected C
warnings and analyzer. A host test can establish a specific invariant for the exercised inputs.
A RIVES smoke run can establish that the packaged cartridge starts and survives a short runtime
path. A visual playthrough can establish properties that none of those automated checks observe.

The validation structure is therefore

~~~math
\boxed{
\text{static compilation}
\longrightarrow
\text{targeted host invariants}
\longrightarrow
\text{RIVES runtime check}
\longrightarrow
\text{human observation}.
}
~~~

Later stages do not turn earlier checks into formal proofs; they provide different evidence about
the same implementation.

## 1. Host compilation

Run

~~~sh
make -C src strict
~~~

Every production source file is compiled separately in three configurations:

- normal release;
- `DEBUG_MODE=1`;
- `CHEATS_ENABLED=1`.

The check uses

~~~text
-std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror -fanalyzer
~~~

and obtains `riv.h` from the installed official RIV SDK. The repository does not vendor that
header; the host checks copy it only into a temporary build directory.

Passing `strict` means that the tested source set is warning-free under this compiler invocation.
It does not establish runtime correctness or absence of undefined behavior outside what the
compiler/analyzer can detect.

## 2. Targeted host invariants

Run

~~~sh
make -C src test
~~~

The host suite uses production functions where possible and small RIVES doubles where platform
state is required.

| Test | Invariant exercised |
| --- | --- |
| `test_room_layout.c` | room coordinates and Manhattan adjacency |
| `test_level_doors.c` | all eleven forward doors, reverse traversal and completed-room backtracking |
| `test_scoring.c` | per-level clocks, bonuses and frozen completed-room scores |
| `test_body_chain.c` | final head anchor and tangent reconstruction from corrected positions |
| `test_spline_math.c` | cubic basis non-negativity, partition of unity and exact values |
| `test_snake_geometry.c` | control count, periodic indexing and stored span samples |
| `test_developer_controls.c` | Technical-view toggle, automatic span selection, manual stepping and wrap-around |
| `test_gameplay_events.c` | final completion path and required-spawn failure handling |
| `test_collision.c` | historical head radius and selected obstacle clearances |

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

These tests support several implementation claims used by the technical documentation.

### Spatial model

The room tests verify the topology used by `LEVEL_MATRIX` and the forward/backward door lifecycle.
They support the distinction between room placement and traversal state, but they do not test
every possible continuous point near a room boundary.

### Procedural body

The chain test verifies that the first joint ends the update at

~~~math
J_0=\Phi(q)
~~~

and that stored tangent angles are reconstructed from the final joint positions. The spline tests
then verify the basis and the periodic sample construction independently.

This is important because the documentation separates the chain from the spline: passing the
spline tests does not imply that every possible joint configuration is dynamically stable.

### Collision geometry

`test_collision.c` fixes the gameplay head radius at

~~~math
r_h=8\cdot0.67=5.36
~~~

and checks selected clearances inherited from the maintained gameplay geometry. It also verifies
that the level-2 square is tested against the same one-pixel inset rectangle that is drawn for the
head/obstacle interaction.

The test does **not** exhaustively validate every collision and response rule described in
[the collision model](collision-model.md).

## 3. What is derived from code but not isolated by a dedicated host test

Several formulas in the documentation are direct translations of production code, yet the
current host suite does not give them their own focused regression test.

This includes:

- the five-point approximation used for head self-collision;
- the exact index range of self-collision segments;
- joint--joint minimum-distance separation and propagated corrections;
- joint--wall correction and propagation;
- body-profile contact that assigns velocity to a stationary collectible;
- collectible confinement to room bounds;
- collectible wall reflection
  
  ~~~math
  v' = 0.8\bigl(v-2(v\cdot n)n\bigr);
  ~~~

- the interaction-specific difference between coin spawn radius 16 and runtime collectible radius
  7.

These statements are documented because they are present in the current implementation. Their
status is **code-derived behavior**, not separately proven subsystem invariants.

## 4. RIVES build and smoke run

Run

~~~sh
make -C src clean all
make -C src smoke
~~~

The latest recorded normal build used RIVEMU/libriv 0.3.0 and RIV OS SDK 0.3.0. Production
modules were compiled with `riv-opt-flags -Ospeed`, the executable was processed by `riv-strip`,
and the packaged cartridge completed a 180-frame headless run at the official 96 MB runtime
limit.

The resulting `snake.sqfs` measured 28,672 bytes, below the recorded 262,144-byte RIVES upload
limit used for that validation.

A smoke run establishes that the packaged artifact can start and execute the exercised short
path. It does not exercise all rooms, collision configurations, collectibles or terminal states.

## 5. Visual and interactive checks

The images in `docs/media/` were captured from the normal build with debug and cheats disabled.
The recorded run checked:

- character selection and start;
- direction changes;
- D/R1 Technical-view toggle;
- A/F and L2/R2 one-span navigation;
- continued movement while the overlay was visible.

The same cartridge was opened in the official web emulator, where selection, start, movement and
Technical-view controls responded as expected.

This layer matters because host tests stub drawing and audio. It can reveal presentation or
interaction failures that mathematical invariants alone do not observe.

## 6. Limits of the current evidence

The present validation does not establish any of the following as a general theorem:

- completeness of the five-sample self-collision approximation for every deformed segment;
- convergence of the body constraint pass;
- absence of all possible joint penetrations after one sequential relaxation pass;
- equivalence between the rendered B-spline surface and collision geometry;
- physical realism of collectible damping or reflection coefficients;
- gameplay difficulty or room-to-room pacing;
- correctness of every concrete RIVES entropy path used by collectible spawning;
- rendered-pixel or audio equivalence across toolchain versions.

In particular,

~~~math
\text{render validation}
\neq
\text{collision validation},
~~~

because the two subsystems intentionally use different geometry.

The documentation therefore uses three kinds of wording deliberately:

- **checked** for behavior exercised by an automated or recorded validation step;
- **implemented** or **code-derived** for behavior read directly from production code;
- **not established** where neither the code structure nor the current tests justify a broader
  claim.

That boundary is part of the technical model: the repository should explain not only what the
code does, but also what the available evidence actually supports.