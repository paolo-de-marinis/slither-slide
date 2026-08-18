# How Slither Slide represents space

Slither Slide uses one global two-dimensional world, but it does not describe that world with
one universal grid. The implementation instead keeps several representations and gives each of
them one precise responsibility.

This is the useful starting point:

~~~math
\text{room topology}
\longrightarrow
\text{logical snake state}
\longrightarrow
\text{world-space geometry}
\longrightarrow
\text{screen-space rendering}.
~~~

The arrows do not mean that one representation replaces the previous one. They mean that a
question is moved into the representation in which it can be answered naturally. A cell of
`LEVEL_MATRIX` names a whole room; a cell of `bodyDirections` stores one successor direction
for the tail; a room-local collectible cell is only a spawn candidate. Walls and body joints,
by contrast, are already geometric objects in continuous world coordinates.

The central design is therefore hybrid:

~~~math
\boxed{\text{discrete state for rules, continuous geometry for shape and contact}.}
~~~

The collision consequences of these representations are derived separately in
[How Slither Slide separates collision, constraints and response](collision-model.md), while the
conversion from the continuous joint chain to the rendered outline is derived in
[Mathematics of the procedural body](b-spline.md).

## 1. Room topology is not a tile map

The first discrete object is [`LEVEL_MATRIX`](../src/room_layout.c):

| `roomY / roomX` | 0 | 1 | 2 | 3 |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 1 | 2 | 11 | 10 |
| 1 | 4 | 3 | 12 | 9 |
| 2 | 5 | 6 | 7 | 8 |

An entry identifies a complete 256 x 256 room. If

~~~math
r=(r_x,r_y)\in\{0,1,2,3\}\times\{0,1,2\},
~~~

then its world-space origin is

~~~math
O(r)=(256r_x,256r_y).
~~~

The twelve rooms therefore occupy the single rectangle

~~~math
\Omega=[0,1024)\times[0,768).
~~~

For example, level 12 lies at matrix coordinate $(2,1)$ and begins at

~~~math
O(2,1)=(512,256).
~~~

The matrix encodes placement, not progression. Two rooms at positions $r$ and $s$ are adjacent
when

~~~math
\lVert r-s\rVert_1
=
\lvert r_x-s_x\rvert+\lvert r_y-s_y\rvert
=1,
~~~

but adjacency alone does not imply that the snake may cross between them.
[`canTraverseLevels()`](../src/room_layout.c) adds the game rule: backward passage to level
$n-1$ is allowed, whereas forward passage to $n+1$ requires the current level to be complete.

Thus two different questions already have two different predicates:

~~~math
\text{where is a room?}
\quad\neq\quad
\text{may the snake enter it?}
~~~

The first is answered by `LEVEL_MATRIX`; the second by the traversal rule.

## 2. The logical snake uses a second discrete space

The logical head position is

~~~math
q=(q_x,q_y)\in\mathbb Z^2.
~~~

A movement tick proposes

~~~math
q'=q+d,
\qquad
d\in\{(-1,0),(1,0),(0,-1),(0,1)\}.
~~~

The pair $q$ is not yet a geometric point. With `TILE_SIZE = 6`, the map

~~~math
\Phi:\mathbb Z^2\to\mathbb R^2,
\qquad
\Phi(q_x,q_y)=(6q_x+3,6q_y+3)
~~~

places the logical cell at its world-space center. For example,

~~~math
\Phi(10,21)=(63,129).
~~~

This distinction is important: the game state moves on the lattice, while collision asks a
question about the image of that state in $\Omega$.

The array

~~~text
bodyDirections[WORLD_TILES_Y][WORLD_TILES_X]
~~~

belongs to the logical layer. At a stored path cell it records the direction that the tail must
follow next; after the tail leaves that cell, the entry is cleared. It is therefore a successor
field for the snake path, not a general occupancy map. Room numbers, walls and collectibles are
not stored in it.

Ceiling division gives the global field the dimensions

~~~math
\left\lceil\frac{1024}{6}\right\rceil
\times
\left\lceil\frac{768}{6}\right\rceil
=171\times128.
~~~

## 3. Room boundaries and snake steps do not share a phase

A room is 256 pixels wide, whereas one logical snake step is 6 pixels:

~~~math
256=42\cdot6+4.
~~~

Consequently a room edge is not, in general, a line of the snake lattice. Near $x=256$, three
successive possible head centers are

~~~math
249,\qquad255,\qquad261.
~~~

The move from 255 to 261 crosses the room boundary in one ordinary logical step. The game does
not restart the lattice in the next room and does not introduce a partial cell. It asks instead
whether the proposed world-space head center may cross the geometric boundary.

For a door whose local interval is $[96,160]$, the allowed center coordinate along the crossed
side is expanded by the head radius $r_h$:

~~~math
96-r_h\leq s\leq160+r_h,
~~~

and the adjacent level must also satisfy the traversal predicate. If the step is accepted,
[`handleLevelTransition()`](../src/levels.c) classifies the new world point by

~~~math
r_x=\left\lfloor\frac{x}{256}\right\rfloor,
\qquad
r_y=\left\lfloor\frac{y}{256}\right\rfloor,
~~~

then reads the new level from `LEVEL_MATRIX[r_y][r_x]`.

A room transition is therefore not a teleport between independent maps. It is the same global
step $q\to q'$ whose geometric image has moved from one 256 x 256 region of $\Omega$ to another.

## 4. Walls are geometric subsets of the world

A wall is stored as an axis-aligned rectangle

~~~c
typedef struct {
    float x;
    float y;
    float width;
    float height;
} Wall;
~~~

and should be read mathematically as

~~~math
W=[x_w,x_w+w]\times[y_w,y_w+h]\subset\Omega.
~~~

The current room boundary is represented by eight rectangles per side. Opening a door removes
the two corresponding central boundary segments. Levels 2--5 append obstacle rectangles. None
of these rectangles is required to align with a 6 x 6 snake cell.

Only the active room's rectangles are retained in `wallState`. Entering another room rebuilds
that set around the new origin $O(r)$. This is a storage choice, not a change of coordinates:
the rectangles themselves remain in the same global world as the snake and collectible.

The basic circle--rectangle contact primitive clamps a circle center $(x_c,y_c)$ to the closest
point of $W$:

~~~math
\begin{aligned}
p_x&=\max(x_w,\min(x_c,x_w+w)),\\
p_y&=\max(y_w,\min(y_c,y_w+h)).
\end{aligned}
~~~

Different subsystems reuse this operation with different radii and different responses. That is
why the wall representation belongs to the spatial model, while the meaning of a contact belongs
to the collision model.

## 5. The collectible changes representation at spawn

The collectible is the clearest example of a state changing mathematical description during its
lifetime.

Spawn first uses a room-local candidate

~~~math
c=(c_x,c_y),
\qquad
1\leq c_x,c_y\leq40,
~~~

because `MAP_SIZE = 42` and only the interior is scanned. Candidates are filtered against walls,
HUD regions, protected corners, the current body and, for a coin, an additional outer margin.
The valid cells are enumerated in a fixed row-major order before RIVES entropy selects one.

For a room at matrix position $r$, the chosen cell is mapped to the world center

~~~math
\Psi_r(c)=O(r)+(6c_x+3,6c_y+3).
~~~

This room-local spawn lattice is not generally the same lattice as $\Phi(\mathbb Z^2)$. If the
room begins at horizontal coordinate 256, then a local candidate has

~~~math
x=256+6c_x+3=259+6c_x\equiv1\pmod6,
~~~

whereas a global snake center has

~~~math
x=3+6q_x\equiv3\pmod6.
~~~

The two lattices therefore need not align. Nothing requires them to: after selection, the
collectible no longer stores $c$ at all. Its persistent state is

~~~math
(C,v,m),
\qquad
C=(x,y)\in\mathbb R^2,
\quad
v=(v_x,v_y)\in\mathbb R^2,
~~~

with the Boolean $m$ recording whether it is moving.

Initially $v=0$. A body contact can assign non-zero velocity, after which each physics update
starts with

~~~math
C\leftarrow C+v,
\qquad
v\leftarrow0.95v.
~~~

The later wall and room responses operate on this continuous state. The spawn cell has served
its purpose and is not an occupancy state to which the object can return.

So the precise statement is not that a collectible "has no cell". It is:

~~~math
\boxed{\text{a cell generates the initial center; continuous state governs the object afterwards}.}
~~~

## 6. The body has a logical anchor and a continuous shape

The logical snake state is sufficient for movement and tail recovery, but not for the curved
body seen on screen. A second description is therefore derived from it:

~~~math
J=(J_0,\ldots,J_{m-1}),
\qquad
J_i=(x_i,y_i)\in\mathbb R^2.
~~~

The first joint is constrained by the logical head:

~~~math
J_0=\Phi(q).
~~~

The remaining joints are not lattice cells. [`bodyChainUpdate()`](../src/body_chain.c) moves them
by following, non-penetration and wall-correction rules, so their coordinates are ordinary
floating-point world positions.

This relation is asymmetric. The logical head determines $J_0$, but the corrected joint chain
does not rewrite $q$. The continuous body is derived from the gameplay state rather than being
its replacement.

Drawing then introduces a final coordinate map. If the camera translation is $K$, then

~~~math
\Pi_K(p)=p-K
~~~

maps a world point to screen coordinates. `snake_char.c` applies this translation to the joint
chain and only then builds the B-spline outline. Since the camera acts after gameplay geometry,
changing the camera cannot alter collision or progression.

The rendered B-spline is likewise downstream from the world-space chain. Self-collision uses a
separate approximation of centerline segments, so changing the number of spline samples can
change the picture without changing the collision rule.

## 7. One world, several authoritative representations

The complete model can now be summarized by the question each representation answers:

| Question | Authoritative representation |
| --- | --- |
| Which level occupies a world region? | room index $r$ and `LEVEL_MATRIX` |
| May the next room be entered? | adjacency + traversal rule |
| Where may the logical head move? | lattice state $q$ and direction $d$ |
| Where is that head geometrically? | world point $\Phi(q)$ |
| How does the tail recover the path? | `bodyDirections` |
| Where are room boundaries and obstacles? | world rectangles $W\in\mathcal W$ |
| Where may a collectible initially appear? | local candidate $c$ mapped by $\Psi_r$ |
| Where is the collectible after spawning? | continuous state $(C,v,m)$ |
| What defines the animated body? | continuous chain $J$ anchored by $J_0=\Phi(q)$ |
| What is finally drawn? | camera-projected controls, B-spline samples and raster primitives |

The corresponding runtime chain is

~~~math
q
\xrightarrow{\text{proposal}}
q'
\xrightarrow{\Phi}
H
\xrightarrow{\text{geometric tests}}
\text{accepted head state}
\xrightarrow{\text{anchor}}
J
\xrightarrow{\text{constraints}}
J'
\xrightarrow{\Pi_K}
\text{render geometry}.
~~~

The collectible evolves alongside this chain in its own continuous state after spawn.

This is why Slither Slide is not well described as "Snake inside one matrix". It uses one world
and several deliberately narrower representations. The mathematical structure comes from the
maps between those representations, not from forcing every subsystem into the same grid.