# Slither Slide code overview

The easiest way to read Slither Slide is not to memorize its modules but to follow the state as it
changes representation.

At a high level, one active frame contains four different kinds of work:

~~~math
\text{logical gameplay}
\longrightarrow
\text{world-space contact}
\longrightarrow
\text{continuous body update}
\longrightarrow
\text{rendering}.
~~~

The code keeps those responsibilities separate even when they operate on the same visible
object. The logical head is not the joint chain; the joint chain is not the B-spline; the
B-spline is not the collider.

The mathematical derivations are split by subject:

- [spatial model](spatial-model.md): which coordinates and state spaces exist;
- [collision model](collision-model.md): which representations interact and how they respond;
- [procedural body](b-spline.md): how the continuous chain becomes the rendered spline.

This document maps those models back to the procedural C implementation.

## 1. Frame-level control flow

`main.c` keeps the application loop short:

~~~c
while (riv_present()) {
    developerControlsUpdate(&developerControls,
                            riv->keys[CONTROL_TECHNICAL_VIEW].press);
    gameUpdate();
    gameDraw(&developerControls);
    playBackgroundMusic();
}
~~~

The main gameplay state is the global `GameData game` declared in `game.c` and described in
`game_state.h`. It contains both discrete and continuous quantities:

- logical head, tail and direction state;
- the global `bodyDirections` successor field;
- the floating-point joint chain;
- movement and animation timers;
- per-level progress and score;
- the current lifecycle state.

The four lifecycle states are character selection, active play, game over and completion.
`gameUpdate()` handles character selection separately, ignores terminal states and then advances
active play in this order:

1. optional cheat handling;
2. snake-only tongue animation;
3. logical movement, collision, room transition and collection;
4. camera update;
5. continuous joint-chain update;
6. scoring update;
7. collectible motion and wall response;
8. body-to-collectible push.

An early return stops the rest of the frame whenever movement or progression produces a terminal
state.

This order is part of the model. In particular, head self-collision uses the candidate new head
against the body chain **before** that frame's chain relaxation.

## 2. Logical movement and its geometric image

`snake_motion.c` owns the discrete head and tail rules. The logical head is an integer lattice
point

~~~math
q=(q_x,q_y)\in\mathbb Z^2,
~~~

and one movement tick proposes

~~~math
q'=q+d,
\qquad
d\in\{(-1,0),(1,0),(0,-1),(0,1)\}.
~~~

The unit in this equation is a **logical step**, not one world-space pixel. The conversion scale
is the implementation constant

~~~math
T=\texttt{TILE\_SIZE}=6.
~~~

The value 6 is not a consequence of the B-spline or of a collision formula. It belongs to the
cartridge's chosen spatial scale. The room-related constants satisfy

~~~math
\texttt{MAP\_SIZE}\,T+\texttt{WALL\_THICKNESS}
=42\cdot6+4
=256
=\texttt{ROOM\_WIDTH}.
~~~

Thus 42 logical increments at this scale cover 252 world-space pixels, while the remaining
4-pixel difference has the same numerical scale as `WALL_THICKNESS`. This relation explains why
`TILE_SIZE = 6` is a natural conversion factor for the cartridge's 256-pixel room geometry; it
should not be read as a universal geometric constant.

The map from a logical cell to the center of its world-space image is therefore

~~~math
\Phi_T(q_x,q_y)
=\left(Tq_x+\frac T2,\ Tq_y+\frac T2\right).
~~~

For the current value $T=6$, this becomes

~~~math
\Phi(q_x,q_y)=(6q_x+3,6q_y+3).
~~~

The offset 3 is simply $T/2$: it places the world-space point at the center of the corresponding
6 x 6 logical step. Consequently the discrete proposal $q\to q+d$ becomes a six-pixel
translation of the candidate head center.

The same scale is retained across the complete global world. Since

~~~math
256=42\cdot6+4
~~~

is not divisible by 6, room edges do not generally coincide with lines of the global snake
lattice. Crossing a room boundary is therefore handled geometrically rather than by restarting
or resizing the lattice in each room.

Before the logical state is changed, `collisionAtNextHead()` maps the proposal to

~~~math
H=\Phi(q')
~~~

and evaluates room-edge, wall and self-collision predicates on that world-space point. Only if
those tests accept the proposal does `snakeMotionUpdate()` commit the new logical state. It then:

1. stores the outgoing direction in `bodyDirections` at the previous head cell;
2. replaces the logical head with $q'$;
3. asks `handleLevelTransition()` whether the new center belongs to another room;
4. checks collection against the continuous collectible center;
5. otherwise advances the tail by reading and clearing its current successor direction.

Thus `bodyDirections` is not a general occupancy grid. It stores only enough information for the
tail to recover the logical path. The important separation is

~~~math
\boxed{\text{integer state decides the path; }\Phi\text{ gives that state a geometric image}.}
~~~

## 3. Rooms, progression and active wall geometry

`room_layout.c` contains the 4 x 3 `LEVEL_MATRIX`. Its responsibility is spatial topology:
level number to room position, inverse lookup and Manhattan adjacency.

`levels.c` adds progression. `canTransitionToLevel()` combines the current level with the
completion rule. `initializeLevel()` then rebuilds the active room state by:

1. obtaining the room origin;
2. calling `wallsBeginRoom()`;
3. removing boundary segments for currently traversable doors;
4. appending level-specific obstacles;
5. starting scoring for the room;
6. spawning a collectible only when the room is not already complete.

The current wall set is therefore local in storage but global in coordinates.

Door opening is represented twice in compatible ways:

- the head boundary predicate permits crossing only through the geometric opening and only when
  progression allows it;
- the corresponding two boundary `Wall` rectangles are removed from the active set.

The first is a gameplay decision; the second changes the active world geometry.

## 4. Collision and contact modules

`collision.c` handles only the head interactions that may reject a proposed logical step. It
does not own every contact in the game.

For the head it evaluates:

- room boundary and door geometry;
- circle--rectangle wall contact;
- five-point sampling of selected joint-chain segments for self-collision.

`walls.c` owns the reusable rectangle representation and the closest-point circle--rectangle
primitive. The same primitive is reused outside `collision.c` for body constraints, collectible
spawn clearance and collectible bounce.

`body_chain.c` then handles a different class of interaction: positional non-penetration. It
separates distant overlapping joints and pushes joints out of walls. These operations do not end
the run and do not reflect velocities because the joints have no velocity state.

`collectible.c` contains still another class: continuous object response. It stores

~~~math
(C,v)
~~~

for the current item, can assign velocity when the body profile touches it and can reflect that
velocity when the item hits room bounds or wall rectangles.

The distinction is summarized in the collision-model document as

~~~math
\text{detection}\neq\text{constraint relaxation}\neq\text{velocity response}.
~~~

## 5. Continuous body chain

After logical movement has been accepted, `body_chain.c` derives the current continuous
centerline.

`bodyChainUpdate()` performs one in-place sequential pass:

~~~math
\text{pin head}
\to
\text{follow previous joint}
\to
\text{joint separation}
\to
\text{wall correction}
\to
\text{pin head again}
\to
\text{recompute tangents}.
~~~

The repeated head pin enforces

~~~math
J_0=\Phi(q)
~~~

as a boundary condition. Other joints may move to arbitrary floating-point coordinates, but the
constraint pass does not rewrite the logical head cell.

`snakeBodyWidth()` is shared by the body constraints, profile construction and some contact
rules. This gives those subsystems a common local width law without making them use the same
collision surface.

## 6. From joint chain to periodic B-spline

Rendering is split into projection, geometric construction and rasterization.

`snake_char.c` first maps every world-space joint through the camera translation and stores the
projected chain in `drawJoints`.

`snake_geometry.c` then constructs the left and right profile offsets and the front control. For
$m$ joints the ideal ordering is

~~~math
L_{m-1},\ldots,L_0,F,R_0,\ldots,R_{m-1}.
~~~

The stored controls are integer-converted screen coordinates. `snakeGeometryEvaluateSpan()`
evaluates one cubic span using four consecutive controls with periodic indexing. Because the
window advances by one control, these are local spans of one global periodic B-spline, not
independent four-control curves.

With `SNAKE_SAMPLES_PER_SPAN = 2`, every span stores the values at

~~~math
t=0,\qquad t=\frac12.
~~~

`snake_char.c` pairs the resulting samples across the ordered outline and fills the body with
triangles. Head, tongue and scale marks are drawn separately.

The caterpillar uses the same continuous joint chain but bypasses this B-spline renderer and has
its own simpler drawing path.

## 7. Collectible lifecycle

The collectible illustrates a representation change in code.

`collectibleSpawn()` first enumerates room-local integer candidates. It filters them against
placement rules, maps the selected candidate into global world coordinates and then discards the
cell identity.

The persistent runtime state contains floating-point position and velocity. A stationary item can
be pushed by two side-profile points derived from the joints. A moving item is advanced, damped,
confined to the room and reflected against active walls.

The implementation uses interaction-specific sizes: coins receive larger spawn clearance and
rendering, while the current runtime collection and bounce rules use the 7-pixel collectible
radius for both item types. The collision-model document records this as an implementation
asymmetry rather than inventing one universal item radius.

## 8. Camera and Technical view

`camera.c` exposes the translation

~~~math
\Pi_K(p)=p-K.
~~~

Gameplay collision and body constraints remain in world coordinates. Camera projection is applied
only for drawing.

`developer_controls.c` keeps Technical-view state outside `GameData`. `technical_view.c` then
receives const pointers to the projected `SnakeGeometry` and live joint data. It visualizes:

- the center chain;
- the complete closed control polygon;
- the selected four-control span window;
- stored renderer samples;
- an explanatory continuously evaluated point $C_i(t)$.

The overlay therefore observes the same geometry used by the renderer without becoming part of
the gameplay state machine.

## 9. Scoring, audio and terminal states

`scoring.c` records per-level active time and completed-room scores. Re-entering a completed room
does not restart its timer or create another collectible.

`audio.c` is the only translation unit that defines `SEQT_IMPL`. Gameplay modules request named
sound effects or start/stop background music; they do not own the sequencer.

`gameEnd()` and `gameComplete()` turn gameplay outcomes into terminal lifecycle states and set the
future quit frame. Once the state is terminal, `gameUpdate()` no longer advances gameplay.

## 10. Build flags and tests

`DEBUG_MODE` and `CHEATS_ENABLED` are independent and default to zero:

- debug mode adds diagnostics;
- cheats enable the R/R3 level-completion helper;
- the Technical view is part of the normal build and depends on neither flag.

The host suite in `tests/` checks selected invariants across room topology, doors, scoring, body
anchoring, B-spline basis and periodic geometry, Technical-view controls, gameplay completion and
head collision clearance.

The suite should not be read as a proof that every continuous configuration is safe. In
particular, several response rules in the collision model are derived directly from production
code but do not yet have dedicated isolated tests. [Validation](validation.md) records the exact
boundary between automated checks, runtime checks and untested behavior.