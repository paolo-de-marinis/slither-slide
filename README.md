# Slither Slide

Slither Slide is a RIVES cartridge built around a multi-room snake game. The player collects apples and coins, grows a procedurally rendered body, opens level doors and avoids walls and self-collision.

## Original RIVES cartridge and Jam result

Slither Slide placed 2nd out of 7 entries in RIVES Jam #3, whose theme was **Slide**.

- [Play the original Slither Slide cartridge on RIVES](https://app.rives.io/cartridges/7654435bf067)
- [Paolo's RIVES profile](https://app.rives.io/profile/0x2e092f91bc25ebd12b8b0e4df87d9d0424d6460c) — the profile lists the two original cartridges
- [RIVES Jam #3 page](https://itch.io/jam/rives-jam-3)
- [Cartesi 2024 year in review — RIVES Creator Season 1 and RIVES Jam #3](https://cartesi.io/blog/goodbye_2024/)
- [Contemporary RIVES post announcing the seven Jam cartridges](https://x.com/rives_io/status/1867175214037291399)
- [RIVES Jam #3 results: Slither Slide in 2nd place](https://x.com/rives_io/status/1867175218642636978)
- Original publication: 3 December 2024, under the RIVES profile **Paolo**

The Slither Slide itch.io submission was later removed, so the live Jam page now shows the six submissions that remain listed. The contemporary RIVES posts record the original seven-cartridge field and Slither Slide's 2nd-place result.

## About this repository

This repository contains the cleaned-up and documented source of the original RIVES cartridge. The gameplay and core implementation derive from the published cartridge; the source was subsequently reorganized and documented to improve readability and make the implementation easier to study. Only the maintained source, standalone host-side tests and required build assets are included; working archives and extracted copies of the original cartridge are intentionally excluded.

## Development

Slither Slide was developed in C against the RIV API (`riv.h`) through a Cursor-assisted workflow. Paolo De Marinis conceived the mechanics and gameplay, directly wrote and modified parts of the code, and handled integration, testing and refinement. He selected and mathematically configured the B-spline curves used for the procedural snake profile; the related implementation code was produced with Cursor assistance and personally integrated and validated.

## Technical reading path

The documentation follows the same progression used in a mathematical implementation note:

1. [Mathematical model](docs/b-spline.md) — assumptions, joint constraints, profile construction, cubic basis, exact midpoint sample and design trade-offs.
2. [Code overview](docs/code-overview.md) — state, frame flow, module responsibilities and correspondence between formulas and functions.
3. [Validation](docs/validation.md) — reproducible checks and the boundary between tested behavior and implementation limits.

## Gameplay

Choose the snake or caterpillar skin, then collect the required items in each room. The last item is rendered as a coin; collecting it completes the room and opens the route forward. Collecting the final coin in room 12 completes the cartridge. Returning to the previous adjacent room is allowed; completed rooms keep their result and do not generate another collectible. Contact between the head and a closed boundary, obstacle or the body ends the run.

| Action | RIVES gamepad |
|---|---|
| Choose skin | D-pad UP / DOWN |
| Start | Any action, shoulder, SELECT or START button |
| Move | D-pad |

The snake cannot reverse directly into its current direction.

## Technical overview

`main()` in `src/main.c` initializes RIVES, audio and `GameData`, then calls `gameUpdate()`, `gameDraw()` and `playBackgroundMusic()` once per `riv_present()` frame. Logical movement uses a tile grid; `JointPoint` values follow the head to produce smooth visual geometry. Levels occupy a 4 × 3 room matrix, with `cameraUpdate()` easing the view between rooms. Head collisions use circle/rectangle and sampled body-segment tests. Rendering converts the two sides of the body into a closed cubic uniform B-spline outline and fills it with triangles.

The implementation is procedural C, not object-oriented code. See [the code overview](docs/code-overview.md) for the complete state and frame flow.

## Code structure

- `src/main.c` — RIVES setup and frame loop.
- `src/game_state.h` — central state and invariants shared by the procedural modules.
- `src/game.c` / `game.h` — lifecycle, frame orchestration and terminal states.
- `src/snake_motion.c` / `snake_motion.h` — input, grid movement, tail motion and collection events.
- `src/collision.c` / `collision.h` — room-edge, wall and self-collision tests.
- `src/body_chain.c` / `body_chain.h` — joint constraints, fixed head anchor and final tangent update.
- `src/scoring.c` / `scoring.h` — per-level clocks, persistent bonuses, total score and outcard.
- `src/game_render.c` / `game_render.h` — HUD, characters, debug geometry and ending panels.
- `src/snake_char.c` / `snake_char.h` — snake profile, cubic B-spline sampling, fill and head/tongue drawing.
- `src/spline_math.c` / `spline_math.h` — the four cubic uniform B-spline basis weights.
- `src/caterpillar_char.c` / `caterpillar_char.h` — alternate character renderer.
- `src/levels.c` / `levels.h` — level requirements, doors, transitions and obstacles.
- `src/room_layout.c` / `room_layout.h` — room matrix and adjacency rules.
- `src/collectible.c` / `collectible.h` — deterministic spawn, collection state and short physics response.
- `src/collectible_render.c` / `collectible_render.h` — apple and coin primitive rendering.
- `src/walls.c` / `walls.h` — wall storage, geometric contact queries and drawing.
- `src/camera.c` / `camera.h` — world-to-screen conversion and room transition smoothing.
- `src/char_selector.c` / `char_selector.h` — initial skin menu.
- `src/audio.c` / `audio.h` and `src/seqt.h` — music and sound effects.
- `tests/` — layout, all door transitions, scoring, final completion, spawn failure, body-chain and B-spline checks.

## Procedural body mathematics

`drawSnakeBody()` creates an ordered closed outline from left and right offsets around the body joints. `drawSmoothCurve()` obtains the four degree-3 uniform B-spline weights from `cubicUniformBSplineBasis()`, samples each span at `t = 0.0` and `0.5`, fills paired samples with triangles and draws the closed edge. This spline is visual geometry; gameplay collision tests use the joint chain and a skin-specific head radius. See [the mathematics of the procedural body](docs/b-spline.md), including the joint constraints, profile construction, cubic basis, design trade-offs and study-source attribution.

## Debug mode

`DEBUG_MODE` in `src/game_state.h` is the compile-time switch. It is `0` by default; set it to `1`, or compile with `-DDEBUG_MODE=1`, to enable collision/audio diagnostics, a geometry overlay and the R3 level-skip helper. The overlay shows joint centers and radii, left/right profile offsets and the skin-specific head collision radius. R3 completes the current level through the ordinary scoring and door path, then advances; on level 12 it completes the run.

## Technical documentation

- [Code overview](docs/code-overview.md)
- [Mathematics of the procedural body and B-spline](docs/b-spline.md)
- [Validation](docs/validation.md)

## Related RIVES cartridge

- [Bomb Flip source repository](https://github.com/paolo-de-marinis/bomb-flip)
- [Play the original Bomb Flip cartridge on RIVES](https://app.rives.io/cartridges/5932d82f5827)

## Prerequisites: RIVEMU and the RIV SDK

This cartridge uses the official [RIV framework and RIVEMU repository](https://github.com/rives-io/riv). RIVEMU is sufficient to run an already-built cartridge; compiling the C sources also requires the RIV SDK.

For complete and platform-specific instructions, see the official RIVES guides:

- [Installing RIVEMU](https://rives.io/docs/riv/getting-started/)
- [Installing the SDK and developing cartridges](https://rives.io/docs/riv/developing-cartridges/)

The following Linux x86_64 setup matches the default paths used by this repository's Makefile:

```sh
mkdir -p "$HOME/.riv"

wget -O "$HOME/.riv/rivemu" \
  https://github.com/rives-io/riv/releases/latest/download/rivemu-linux-amd64
chmod +x "$HOME/.riv/rivemu"

wget -O "$HOME/.riv/rivos-sdk.ext2" \
  https://github.com/rives-io/riv/releases/latest/download/rivos-sdk.ext2
```

Verify both components:

```sh
"$HOME/.riv/rivemu" -version

RIVEMU_SDK="$HOME/.riv/rivos-sdk.ext2" \
  "$HOME/.riv/rivemu" -quiet -no-window -sdk \
  -exec /usr/lib/libriv.so version
```

The source preceding the current module-and-behavior refactor was verified with RIVEMU and `libriv` 0.3.0. The maintained refactor passes the host checks described below; its RISC-V package and smoke run remain release checks, as stated in [Validation](docs/validation.md). For another operating system or architecture, download the matching RIVEMU binary from the [official releases](https://github.com/rives-io/riv/releases) and pass its path to `make`.

## Building and running

By default, the Makefile reads RIVEMU and the SDK from `~/.riv`. To use different locations, override `RIVEMU` and `RIVEMU_SDK`:

```sh
make -C src \
  RIVEMU=/path/to/rivemu \
  RIVEMU_SDK=/path/to/rivos-sdk.ext2 \
  clean all
```

With the default installation above:

```sh
make -C src clean all
make -C src run
```

Checks:

```sh
make -C src strict
make -C src test
make -C src smoke
```

`strict` checks all production modules with a warning-free C11 analysis in both debug configurations. `test` builds six host-side C checks for the room matrix, all eleven door transitions and backtracking, per-level scoring, body-chain invariants, the cubic B-spline basis, final completion and spawn failure. `smoke` runs the packaged cartridge headlessly for 180 frames.

## Repository history

This repository starts from the reviewed, publication-ready source. The published 2024 RIVES cartridge remains available through the RIVES link above, while local working archives and cleanup history are intentionally not part of this repository. The later cleanup does not establish historical authorship of individual parts of the original cartridge.

## License

No license file was included with the original cartridge source, and this repository does not add one. No permission to copy, modify or redistribute the source is granted beyond rights provided by applicable law.
