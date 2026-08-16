# Slither Slide

![Slither Slide: procedural snake gameplay and runtime Technical view](docs/media/slither-slide-hero.png)

Slither Slide is a multi-room snake game built as a RIVES cartridge. Its twelve rooms occupy a
4 × 3 lattice inside one 1024 × 768 world. The head moves on a global tile grid, while walls,
collectible physics and a chain of floating-point joints use geometric world coordinates. A
closed cubic B-spline gives the projected body its curved outline.

The cartridge placed **2nd out of 7 entries** in RIVES Jam #3 (theme: *Slide*).

- [Play the original cartridge on RIVES](https://app.rives.io/cartridges/7654435bf067)
- [RIVES Jam #3](https://itch.io/jam/rives-jam-3)
- [RIVES Jam #3 results](https://x.com/rives_io/status/1867175218642636978)
- [Cartesi's 2024 RIVES recap](https://cartesi.io/blog/goodbye_2024/)

The itch.io submission was later removed, so the Jam page now lists six surviving submissions. The contemporary RIVES posts record the original seven entries and the result.

## What is in this repository

This is the maintained, post-jam version of the cartridge. The exact source snapshot submitted in December 2024 is no longer retained. The present code includes later fixes, tests, documentation and refactoring; the original published cartridge remains available at the RIVES link above.

The original game was developed in C with Cursor assistance. Paolo De Marinis designed the mechanics and gameplay, wrote and modified parts of the code, and handled integration, testing and refinement. He selected and configured the B-spline construction used for the snake profile; its implementation was produced with assistance and then integrated and validated by him. Since 2026, OpenAI Codex has assisted with repository maintenance, including refactoring, tests and documentation. Paolo reviewed, integrated and validated these changes.

## Gameplay and controls

Choose the snake or caterpillar, collect the required items in each room and pass through the doors that open. The last item in a room is a coin. Completed rooms remain completed when the player backtracks. The run ends on contact with a closed boundary, an obstacle or the snake's body.

| Action | Keyboard | RIVES gamepad |
| --- | --- | --- |
| Choose skin | Up / Down | D-pad Up / Down |
| Start | Z or E | A1 or Start |
| Move | Arrow keys | D-pad |
| Toggle Technical view | D | R1 |
| Previous / next B-spline span | A / F | L2 / R2 |

The Technical view is part of the normal build and starts disabled. It is available only for the snake skin.

![Slither Slide Technical view](docs/media/slither-slide-technical-view.gif)

Light-blue points are the complete closed control polygon. The four orange rings identify the four controls used by the selected span; they are not the only control points in the body. The white point moves along that span, while the yellow points show the two samples per span used by the renderer. Before the first A/F or L2/R2 press, the selected span advances automatically.

## Reading the code

The shortest path through the program is:

1. `src/main.c` sets up RIVES and runs one update/draw/audio cycle per frame.
2. `src/game.c` controls the four game states and the order of the update.
3. `src/game_state.h` contains the state shared by the gameplay modules.
4. `src/snake_motion.c` updates the logical grid snake.
5. `src/body_chain.c`, `src/snake_geometry.c` and `src/snake_char.c` turn that state into the visible body.

The remaining modules have narrow responsibilities:

- `levels.c`, `room_layout.c` and `walls.c` define the twelve-room world and its doors;
- `collision.c` checks the next head position against boundaries, walls and body segments;
- `collectible.c` and `collectible_render.c` manage apples and coins;
- `scoring.c` records per-level time and score;
- `technical_view.c` displays the same geometry used by the renderer without changing gameplay state;
- `camera.c`, `game_render.c`, `char_selector.c` and `audio.c` handle presentation.

The code is procedural C. State is explicit, functions are grouped by responsibility, and the frame order can be followed without an object hierarchy.

## How space is represented

The game has one global 2D world, but not one global tile map. `LEVEL_MATRIX` says where the
twelve complete rooms are placed. The separate `bodyDirections` array records only the path
that the snake's tail must follow; it contains neither walls nor collectibles.

If the logical head occupies integer cell $q=(q_x,q_y)$, its geometric world center is

~~~math
\Phi(q)=(6q_x+3,\ 6q_y+3).
~~~

Walls are tested as rectangles against this mapped point. A collectible is initially chosen
from a room-local grid, but the selected center is immediately converted to floating-point
position and velocity. The curved body is similarly derived from world-space floating-point
joints rather than from occupied grid cells.

These distinctions matter because a 256-pixel room is not an integer number of 6-pixel snake
steps: $256=42\cdot6+4$. The room boundary, global snake grid and local spawn grid therefore
need not align; doors, collection and collision join them through geometric tests.

See [How Slither Slide represents space](docs/spatial-model.md) for the derivation from the room
matrix to the global world, a complete door-crossing example, the collectible's change of
representation and the connection with the procedural body.

## The B-spline construction

For a body joint $J_i$, local direction $\theta_i$ and half-width $w_i$, the two outline points are

~~~math
L_i=J_i+w_i(-\sin\theta_i,\cos\theta_i),\qquad
R_i=J_i-w_i(-\sin\theta_i,\cos\theta_i).
~~~

The ordered left and right profiles form a closed control polygon. Each cubic uniform B-spline span uses four consecutive controls:

~~~math
C_i(t)=\sum_{k=0}^{3} b_k(t)P_{i+k},\qquad 0\leq t<1.
~~~

Control indices wrap around the polygon. The renderer samples each span at $t=0$ and $t=0.5$, then fills the body with paired triangles. Collision detection deliberately uses the joint chain and simpler primitives instead of the rendered triangles.

The full derivation, including the joint constraints, basis functions and midpoint sample, is in [Mathematics of the procedural body](docs/b-spline.md).

## Building

Running an existing cartridge requires [RIVEMU](https://rives.io/docs/riv/getting-started/). Compiling the C source also requires the [RIV SDK](https://rives.io/docs/riv/developing-cartridges/). The Makefile uses these default paths:

~~~text
~/.riv/rivemu
~/.riv/rivos-sdk.ext2
~~~

The repository deliberately does not vendor `riv.h`; builds use the API header supplied by the installed RIV SDK. Host-side `strict` and `test` checks copy that SDK header only into a temporary build directory.

Build and run:

~~~sh
make -C src clean all
make -C src run
~~~

For different locations:

~~~sh
make -C src \
  RIVEMU=/path/to/rivemu \
  RIVEMU_SDK=/path/to/rivos-sdk.ext2 \
  clean all
~~~

Local checks:

~~~sh
make -C src strict
make -C src test
make -C src smoke
~~~

`strict` compiles every production module as warning-free C11 in release, debug and cheats configurations. `test` runs nine host-side checks. `smoke` starts the packaged cartridge headlessly for 180 frames. The latest recorded RIVES build and runtime check used RIVEMU/libriv and RIV OS SDK 0.3.0; see [Validation](docs/validation.md).

## Documentation

- [How Slither Slide represents space](docs/spatial-model.md)
- [Mathematics of the procedural body](docs/b-spline.md)
- [Code overview](docs/code-overview.md)
- [Validation](docs/validation.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)
- [SEQT GPLv3 §7 exception](SEQT_EXCEPTION.md)

Related project: [Bomb Flip source](https://github.com/paolo-de-marinis/bomb-flip) · [original Bomb Flip cartridge](https://app.rives.io/cartridges/5932d82f5827)

## License

Except for the third-party material listed in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md), the source, documentation and original assets are Copyright © 2024–2026 Paolo De Marinis and licensed under the [GNU General Public License, version 3 or later](LICENSE), with the narrow [SEQT additional permission](SEQT_EXCEPTION.md).
