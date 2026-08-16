# How Slither Slide represents space

Slither Slide has **one global 2D world, but not one global tile map**. This is the central
distinction behind the code.

In concrete terms, a cell of `LEVEL_MATRIX` identifies a whole room; a cell of
`bodyDirections` records one direction of the snake's logical path; a cell considered during
collectible spawning is only a temporary candidate. Treating all three as the same kind of
cell is what makes the implementation appear more confusing than it is.

The room matrix says where complete rooms are placed. A different grid records the logical
path followed by the snake. Walls are rectangles, the visible body is derived from a
floating-point joint chain, and a collectible uses a grid only to choose its initial position.
After that choice, the collectible is an ordinary moving object with continuous position and
velocity.

The result is a hybrid discrete/continuous model: discrete where the rules require exact
steps, continuous where the game requires geometry, curved animation or physical response.

## 1. From the room matrix to one world

The first discrete structure is [`LEVEL_MATRIX`](../src/room_layout.c). Its entries are

| `roomY / roomX` | 0 | 1 | 2 | 3 |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 1 | 2 | 11 | 10 |
| 1 | 4 | 3 | 12 | 9 |
| 2 | 5 | 6 | 7 | 8 |

An entry is not a game tile. It identifies an entire 256 × 256 room. If
$r=(r_x,r_y)$ is a position in this matrix, the corresponding room begins at the global
world coordinate

~~~math
O(r)=O(r_x,r_y)=(256r_x,256r_y).
~~~

For example, matrix position $(2,1)$ contains level 12 and has origin

~~~math
O(2,1)=(512,256).
~~~

Placing four rooms horizontally and three vertically therefore produces the single world

~~~math
[0,1024)\times[0,768).
~~~

[`getRoomPosition()`](../src/room_layout.c) maps a level number to its matrix position, while
[`getLevelAtPosition()`](../src/room_layout.c) performs the inverse lookup. Two rooms are
adjacent when their matrix positions have Manhattan distance one:

~~~math
|r_x-s_x|+|r_y-s_y|=1.
~~~

The numbering $1,2,\ldots,12$ follows a path through adjacent rooms. Forward passage from
level $n$ to level $n+1$ is enabled only after level $n$ is complete; passage back to level
$n-1$ remains possible. Thus `LEVEL_MATRIX` determines spatial placement, whereas
[`canTraverseLevels()`](../src/room_layout.c) determines progression. These are related, but
they are not the same rule.

Once the rooms have been placed in a common world, the snake must also use coordinates that
continue across room boundaries. This leads to the second discrete structure.

## 2. From a snake cell to a world point

The logical head position is an integer pair

~~~math
q=(q_x,q_y)\in\mathbb Z^2.
~~~

With `TILE_SIZE = 6`, [`snakeMotionUpdate()`](../src/snake_motion.c) advances it by one
cardinal direction:

~~~math
q'=q+d,
\qquad
d\in\{(-1,0),(1,0),(0,-1),(0,1)\}.
~~~

The integer pair is not itself a pixel position. Whenever the game needs the geometric center
of the head, it applies

~~~math
\Phi(q_x,q_y)=(6q_x+3,\ 6q_y+3).
~~~

For instance, the logical cell $(10,21)$ corresponds to the world point

~~~math
\Phi(10,21)=(63,129).
~~~

The array

~~~text
bodyDirections[WORLD_TILES_Y][WORLD_TILES_X]
~~~

belongs only to this logical description. At a cell occupied by the stored path, it records
the cardinal direction that the tail must follow next. When the tail leaves that cell, the
entry is cleared. The array does **not** contain room numbers, walls or collectibles; it is a
direction field for the snake's path, not a general occupancy map.

Ceiling division gives this global direction field the dimensions

~~~math
\left\lceil\frac{1024}{6}\right\rceil
\times
\left\lceil\frac{768}{6}\right\rceil
=171\times128.
~~~

There is an important consequence. A room width is not an integer number of snake steps:

~~~math
256=42\cdot6+4.
~~~

Therefore a room boundary is not a line of the snake grid. Near the vertical boundary
$x=256$, three consecutive head centers are

~~~math
249,\qquad255,\qquad261.
~~~

The step from $255$ to $261$ crosses the boundary. The game does not invent a partial cell or
restart the grid in the next room. It tests the proposed world point against the door and, if
passage is allowed, accepts the same global snake step.

More precisely, [`collidesWithRoomEdge()`](../src/collision.c) takes the coordinate $s$ along
the crossed side and accepts the head center inside the radius-expanded door interval

~~~math
96-r_h\leq s\leq160+r_h,
~~~

provided that the adjacent level is traversable. Here $r_h$ is the head collision radius.

After the step, [`handleLevelTransition()`](../src/levels.c) determines the containing room
from the new head center $(x,y)$:

~~~math
r_x=\left\lfloor\frac{x}{256}\right\rfloor,
\qquad
r_y=\left\lfloor\frac{y}{256}\right\rfloor.
~~~

The new level is then `LEVEL_MATRIX[r_y][r_x]`. Room transition is consequently not a
teleport between separate maps: it is a discrete step whose geometric center has entered a
different region of the same world.

## 3. Why walls are not snake cells

Once a proposed snake cell has been converted by $\Phi$, collision can be computed directly
in world coordinates. A wall is stored as the complete axis-aligned rectangle

~~~c
typedef struct {
    float x;
    float y;
    float width;
    float height;
} Wall;
~~~

The boundary of the current room is divided into eight rectangles per side. Removing the two
central segments, with local interval $[96,160]$, opens a door. Levels 2–5 then add obstacle
rectangles of different sizes and positions. None of these rectangles has to coincide with a
6 × 6 snake cell.

For a circle with center $(x_c,y_c)$ and a wall beginning at $(x_w,y_w)$, with width $w$ and
height $h$, the closest point of the rectangle is

~~~math
\begin{aligned}
c_x&=\max(x_w,\min(x_c,x_w+w)),\\
c_y&=\max(y_w,\min(y_c,y_w+h)).
\end{aligned}
~~~

[`wallCircleContact()`](../src/walls.c) compares the distance from $(x_c,y_c)$ to
$(c_x,c_y)$ with the required radius. This single geometric operation is reused for head
collision, joint correction, collectible spawn clearance and collectible bounce response.

Only the current room's boundary and obstacle rectangles are kept in `wallState`. Entering a
room rebuilds that active set using the room origin $O(r)$. The coordinates remain global even
though inactive rooms do not need to keep their rectangles in memory.

The wall model is therefore continuous and local to the active room, whereas the snake path
grid is discrete and global. The collectible connects these two descriptions in a slightly
different way.

## 4. The collectible is discrete only when it is spawned

[`collectibleSpawn()`](../src/collectible.c) first enumerates possible positions using an
integer pair $c=(c_x,c_y)$ local to the current room. With `MAP_SIZE = 42`, the scanned
interior indices are

~~~math
1\leq c_x,c_y\leq40.
~~~

A candidate is rejected if it intersects a wall plus clearance, the HUD, a protected corner,
the current joint chain or, for a coin, the additional outer margin. Candidates are enumerated
in a fixed row-major order; RIVES entropy then selects one of the valid entries.

If the current room is at matrix position $r$, the selected local candidate becomes the
world-space center

~~~math
\Psi_r(c)=O(r)+(6c_x+3,\ 6c_y+3).
~~~

This spawn lattice is not generally the global snake lattice. Since the room origin can be
256 or 512 pixels from the world origin, adding $O(r)$ changes the coordinate phase modulo 6.
For example, in a room with horizontal origin 256, a local candidate center has coordinate

~~~math
x=256+6c_x+3=259+6c_x,
~~~

whereas every global snake center has the form $3+6q_x$. Their residues modulo 6 are therefore
1 and 3: the two lattices do not coincide. Exact alignment is unnecessary, because contact
with the head is tested by continuous distance rather than equality of cell indices.

Immediately after selection, the game stores the collectible as

~~~c
typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    bool moving;
} PhysicsObject;
~~~

Initially $(v_x,v_y)=(0,0)$. If a side point of the snake profile overlaps the collectible,
[`collectiblePushFromBody()`](../src/collectible.c) assigns a nonzero velocity. Each later
physics update applies

~~~math
\begin{aligned}
x&\leftarrow x+v_x,\\
y&\leftarrow y+v_y,\\
(v_x,v_y)&\leftarrow0.95(v_x,v_y),
\end{aligned}
~~~

followed by collision response against room bounds and wall rectangles. The position is never
snapped back to the selected spawn cell.

This is the precise correction to the oversimplified statement that a collectible “does not
occupy a cell”: a cell is used to **generate its initial center**, but no cell coordinate is
kept as its subsequent state. Collection occurs when the circle around the continuous
collectible center overlaps the circle around the mapped head center $\Phi(q)$.

## 5. The snake also has a discrete and a continuous description

The logical snake described by $q$ and `bodyDirections` is sufficient for movement and tail
growth, but it cannot produce the curved body shown on screen. The game therefore derives a
second description from it.

Let

~~~math
J_i=(x_i,y_i),\qquad i=0,\ldots,m-1,
~~~

be the floating-point joints of the body in world coordinates. The first joint is constrained
by the logical head:

~~~math
J_0=\Phi(q).
~~~

The other joints are not grid cells. [`bodyChainUpdate()`](../src/body_chain.c) lets them
follow one another, separates distant overlapping joints and pushes them away from wall
rectangles. These corrections can leave a joint at any floating-point coordinate.

Drawing introduces one final conversion. If the camera position is $K$, then
[`worldToScreen()`](../src/camera.c) applies

~~~math
p_{\mathrm{screen}}=p_{\mathrm{world}}-K.
~~~

[`snake_char.c`](../src/snake_char.c) projects the joints first and then passes the projected
chain to [`snakeGeometryBuild()`](../src/snake_geometry.c). Left and right offsets around the
joints become the closed control polygon; the periodic cubic B-spline is evaluated in screen
coordinates and rasterized with triangles.

The camera and B-spline do not feed back into the movement grid. Self-collision uses samples
of world-space centerline segments rather than the rendered triangles. Thus changing the
number of spline samples can change the picture without silently changing the game rules.
The full construction is derived in
[Mathematics of the procedural body](b-spline.md).

## 6. The complete model

The representations can now be summarized without conflating their roles:

| Question | Authoritative representation |
| --- | --- |
| Which level occupies a region of the world? | `LEVEL_MATRIX` and room index $r$ |
| Where does the logical head move next? | global snake cell $q$ and cardinal direction $d$ |
| How does the tail recover the stored path? | `bodyDirections` |
| Where is the head geometrically? | mapped world point $\Phi(q)$ |
| Where are boundaries and obstacles? | world-space `Wall` rectangles |
| Where may an item initially appear? | room-local candidate $c$ mapped by $\Psi_r$ |
| Where is the item after spawning? | floating-point `PhysicsObject` state |
| What defines the animated body? | world-space joint chain $J_0,\ldots,J_{m-1}$ |
| What is finally drawn? | camera-projected B-spline samples and raster primitives |

During a movement frame, the code therefore performs the following chain:

1. propose a new integer snake cell;
2. map it through $\Phi$ and test it against world-space geometry;
3. update the logical path and, when necessary, the current room;
4. pin the head joint to the mapped center and correct the continuous chain;
5. advance the collectible's continuous physics;
6. project the resulting world geometry through the camera and draw it.

Slither Slide is not “Snake inside one matrix”, nor does it need a single higher-dimensional
array containing every object. It uses one global world together with the smallest
representation required by each subsystem: room indices for topology, integer cells for
movement, and continuous geometry for shape, collision and physics.
