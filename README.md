# Slither Slide

![Slither Slide: procedural snake gameplay and runtime Technical view](docs/media/slither-slide-hero.png)

Slither Slide is a multi-room snake game built as a RIVES cartridge. Its twelve rooms occupy a
4 x 3 arrangement inside one 1024 x 768 world, but the game does **not** represent that world
with one universal tile map.

> **Development model.** This is an AI-assisted C project. Cursor substantially assisted the
> original implementation; Paolo De Marinis designed the mechanics and gameplay, selected and
> configured the B-spline construction, wrote or modified parts of the code, and handled
> integration, testing and validation. Since 2026, OpenAI Codex has assisted with repository
> maintenance, refactoring, tests and documentation.

The implementation separates the questions it has to answer:

~~~math
\text{logical movement}
\longrightarrow
\text{world-space geometry}
\longrightarrow
\text{continuous body constraints}
\longrightarrow
\text{rendered B-spline}.
~~~

The head moves on a global 6-pixel lattice. Room boundaries and obstacles are rectangles in
world coordinates. The animated body is a floating-point joint chain anchored to the logical
head. A closed periodic cubic B-spline is built from that chain only for rendering. Collision
uses simpler interaction-specific geometry rather than the filled spline surface.

The cartridge placed **2nd out of 7 entries** in RIVES Jam #3 (theme: *Slide*).

- [Play the original cartridge on RIVES](https://app.rives.io/cartridges/7654435bf067)
- [RIVES Jam #3](https://itch.io/jam/rives-jam-3)
- [RIVES Jam #3 results](https://x.com/rives_io/status/1867175218642636978)
- [Cartesi's 2024 RIVES recap](https://cartesi.io/blog/goodbye_2024/)

The itch.io submission was later removed, so the Jam page now lists six surviving submissions.
Contemporary RIVES posts record the original seven entries and the result.

## What is in this repository

This is the maintained post-jam version of the cartridge. The exact source snapshot submitted in
December 2024 is no longer retained. The present repository contains later fixes, tests,
documentation and refactoring; the original published cartridge remains available at the RIVES
link above.

The original game was developed in C with Cursor assistance. Paolo De Marinis designed the
mechanics and gameplay, wrote and modified parts of the code, and handled integration, testing
and refinement. He selected and configured the B-spline construction used for the snake profile;
its implementation was produced with assistance and then integrated and validated by him. Since
2026, OpenAI Codex has assisted with repository maintenance, including refactoring, tests and
documentation. Paolo reviewed, integrated and validated these changes.

## Gameplay and controls

Choose the snake or caterpillar, collect the required items in each room and pass through the
doors that open. The last required item in a room is a coin. Completed rooms remain completed
when the player backtracks. The run ends when the proposed head step collides with a closed
boundary, an obstacle or the relevant part of the snake's body.

| Action | Keyboard | RIVES gamepad |
| --- | --- | --- |
| Choose skin | Up / Down | D-pad Up / Down |
| Start | Z or E | A1 or Start |
| Move | Arrow keys | D-pad |
| Toggle Technical view | D | R1 |
| Previous / next B-spline span | A / F | L2 / R2 |

The Technical view is part of the normal build and starts disabled. It is available only for the
snake skin.

![Slither Slide Technical view](docs/media/slither-slide-technical-view.gif)

Light-blue points are the complete closed control polygon. The four orange rings are the local
four-control window used by the selected cubic span; they are not the only controls in the body.
The white point evaluates that span continuously for explanation, while the yellow points are the
two samples per span actually stored for rendering.

## Reading the model

The documentation is organized by mathematical responsibility rather than by source-file order.

1. [How Slither Slide represents space](docs/spatial-model.md) derives the maps between room
   topology, the logical snake lattice, world coordinates, collectible state and camera space.
2. [How Slither Slide separates collision, constraints and response](docs/collision-model.md)
   distinguishes gameplay collision, body non-penetration and collectible physics, and derives
   the geometry used by each interaction.
3. [Mathematics of the procedural body](docs/b-spline.md) starts from the continuous joint chain,
   constructs the left/right profile and derives the single closed periodic cubic B-spline used
   by the renderer.
4. [Code overview](docs/code-overview.md) maps those mathematical roles back to the procedural C
   modules and frame order.
5. [Validation](docs/validation.md) separates what is checked automatically from what still
   requires runtime or visual observation.

The intended reading direction is therefore

~~~math
\boxed{
\text{space}
\longrightarrow
\text{interaction}
\longrightarrow
\text{shape}
\longrightarrow
\text{implementation}
\longrightarrow
\text{validation}.
}
~~~

## Spatial model in one page

The twelve rooms are placed by `LEVEL_MATRIX`. A matrix entry names one complete 256 x 256 room;
it is not a gameplay tile. The snake has a separate logical position

~~~math
q=(q_x,q_y)\in\mathbb Z^2
~~~

and the world-space center corresponding to that position is

~~~math
\Phi(q)=(6q_x+3,6q_y+3).
~~~

The tail path is stored independently in `bodyDirections`. Walls are continuous rectangles.
Collectibles use a room-local lattice only to choose an initial center and then retain
floating-point position and velocity. The animated body is the joint chain

~~~math
J=(J_0,\ldots,J_{m-1}),
\qquad
J_0=\Phi(q).
~~~

A 256-pixel room is not an integer number of 6-pixel head steps:

~~~math
256=42\cdot6+4.
~~~

Room edges, the global head lattice and room-local spawn candidates therefore do not generally
align. Door crossing, collection and collision connect them through world-space geometry rather
than through a shared cell index.

## Collision is not one algorithm

The game deliberately uses different representations for interactions with different semantics.
For example:

- head against wall: circle--rectangle contact, with game over as the response;
- head against body: five samples on selected centerline segments, each thickened by the head and
  body radii;
- joint against joint: positional separation with no velocity model;
- joint against wall: positional correction;
- body against collectible: side-profile contact that assigns velocity to a stationary item;
- collectible against wall: circle--rectangle contact followed by damped velocity reflection;
- head against collectible: circle--circle overlap interpreted as collection.

The full distinction is derived in the collision model. In particular, the five self-collision
samples **discretize the centerline segment**, while the radius $r_h+w_i$ gives that sampled
segment its effective thickness. They are an approximation of a thick segment, not five
independent body joints.

## Procedural body in one page

For a world-space joint $J_i$, tangent angle $\theta_i$ and local half-width $w_i$, define

~~~math
\mathbf n_i=(-\sin\theta_i,\cos\theta_i),
~~~

then

~~~math
L_i=J_i+w_i\mathbf n_i,
\qquad
R_i=J_i-w_i\mathbf n_i.
~~~

Together with a front point $F$, these offsets are ordered around the boundary as

~~~math
L_{m-1},\ldots,L_0,F,R_0,\ldots,R_{m-1}.
~~~

They form one periodic control sequence. A cubic span uses four consecutive controls because a
degree-3 B-spline has local support:

~~~math
C_i(t)=\sum_{k=0}^{3}b_k(t)P_{i+k},
\qquad0\leq t<1,
~~~

with periodic indices. The four-point window therefore evaluates a local span of the **same
global B-spline**; it is not a separate four-point spline.

The renderer samples each span at $t=0$ and $t=1/2$ and fills the resulting finite outline with
triangles. Collision remains independent of that rasterization.

## Reading the code

The shortest implementation path is:

1. `src/main.c` owns the RIVES frame loop.
2. `src/game.c` orders gameplay updates.
3. `src/game_state.h` contains the shared logical and continuous state.
4. `src/snake_motion.c` proposes and accepts logical head movement.
5. `src/collision.c`, `src/walls.c` and `src/levels.c` evaluate head/world interactions.
6. `src/body_chain.c` updates the continuous centerline.
7. `src/snake_geometry.c` evaluates the periodic cubic B-spline.
8. `src/snake_char.c` projects and rasterizes the visible snake.
9. `src/collectible.c` owns spawn, continuous collectible state and its local physics.

The remaining modules keep presentation, scoring, character selection and audio separate. The
program is procedural C: state is explicit, functions are grouped by responsibility and the
frame order remains visible without an object hierarchy.

## Building

Running an existing cartridge requires [RIVEMU](https://rives.io/docs/riv/getting-started/).
Compiling the C source also requires the [RIV SDK](https://rives.io/docs/riv/developing-cartridges/).
The Makefile uses these default paths:

~~~text
~/.riv/rivemu
~/.riv/rivos-sdk.ext2
~~~

The repository deliberately does not vendor `riv.h`; builds use the API header supplied by the
installed RIV SDK. Host-side `strict` and `test` checks copy that SDK header only into a temporary
build directory.

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

`strict` compiles every production module as warning-free C11 in release, debug and cheats
configurations. `test` runs the host-side checks. `smoke` starts the packaged cartridge
headlessly for 180 frames. The latest recorded RIVES build and runtime check used
RIVEMU/libriv and RIV OS SDK 0.3.0; see [Validation](docs/validation.md).

## Documentation

- [How Slither Slide represents space](docs/spatial-model.md)
- [How Slither Slide separates collision, constraints and response](docs/collision-model.md)
- [Mathematics of the procedural body](docs/b-spline.md)
- [Code overview](docs/code-overview.md)
- [Validation](docs/validation.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)
- [SEQT GPLv3 §7 exception](SEQT_EXCEPTION.md)

Related project: [Bomb Flip source](https://github.com/paolo-de-marinis/bomb-flip) · [original Bomb Flip cartridge](https://app.rives.io/cartridges/5932d82f5827)

## License

Except for the third-party material listed in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md),
the source, documentation and original assets are Copyright © 2024--2026 Paolo De Marinis and
licensed under the [GNU General Public License, version 3 or later](LICENSE), with the narrow
[SEQT additional permission](SEQT_EXCEPTION.md).