# How Slither Slide separates collision, constraints and response

Slither Slide does not have one universal collision mesh. It has several interactions whose
meanings are different enough that using the same geometric model for all of them would hide the
actual structure of the game.

The useful distinction is between three steps:

~~~math
\boxed{\text{representation}\longrightarrow\text{contact predicate}\longrightarrow\text{response}.}
~~~

A contact predicate answers whether two represented objects overlap. A response says what that
overlap means for the state. These are not the same question.

For the head, contact with a wall or the body can terminate the run. For two body joints,
contact causes a positional separation. For the collectible, contact can assign or reflect a
velocity. The filled B-spline triangles are not authoritative for any of these rules.

The model can be summarized before looking at the individual formulas:

| Interaction | Representation | Contact meaning | Response |
| --- | --- | --- | --- |
| head--room edge | head circle + room bounds | proposed step crosses a closed boundary | reject step / game over |
| head--wall | circle + rectangle | proposed head overlaps wall geometry | reject step / game over |
| head--body | head circle + sampled thick centerline | proposed head enters its own body | reject step / game over |
| joint--joint | two joint centers + exclusion distance | distant parts of body overlap | positional separation |
| joint--wall | joint circle + rectangle | body chain penetrates a wall | positional correction |
| head--collectible | two circles | item is reached | collection event |
| body--collectible | side-profile point + collectible radius | body pushes a stationary item | assign velocity |
| collectible--wall | circle + rectangle + velocity | moving item penetrates wall | reposition + damped reflection |

The spatial objects themselves are derived in
[How Slither Slide represents space](spatial-model.md). The rendered body constructed from the
joint chain is derived in [Mathematics of the procedural body](b-spline.md).

## 1. Contact predicates and state responses

It is useful to write a generic contact test as

~~~math
D(x,y)\in\{0,1\}.
~~~

The same value $D=1$ can lead to very different state transitions. For example,

~~~math
D_{\mathrm{head,wall}}(H,W)=1
\quad\Longrightarrow\quad
\text{terminal gameplay transition},
~~~

whereas

~~~math
D_{\mathrm{joint,joint}}(J_i,J_j)=1
\quad\Longrightarrow\quad
(J_i,J_j)\mapsto(J_i',J_j').
~~~

The first interaction has no geometric bounce because the proposed movement is simply not
accepted. The second has no momentum law because the joints do not store velocities. The
collectible is different again because its state explicitly includes velocity:

~~~math
(C,v)\mapsto(C',v').
~~~

This is the reason the implementation uses several contact models: each interaction keeps only
the geometric and dynamical quantities needed by its own rule. The code does not attempt to
simulate one global rigid-body system.

## 2. The shared circle--rectangle primitive

Walls and obstacles are axis-aligned rectangles in world coordinates. Let

~~~math
W=[x_w,x_w+w]\times[y_w,y_w+h]
~~~

and let a circle have center

~~~math
c=(x_c,y_c)
~~~

and radius $r$. [`wallCircleContact()`](../src/walls.c) first finds the closest rectangle point
by coordinatewise clamping:

~~~math
\begin{aligned}
p_x&=\max(x_w,\min(x_c,x_w+w)),\\
p_y&=\max(y_w,\min(y_c,y_w+h)).
\end{aligned}
~~~

With

~~~math
p=(p_x,p_y),
\qquad
\delta=c-p,
\qquad
d=\lVert\delta\rVert,
~~~

contact is reported when

~~~math
d^2<r^2.
~~~

The inequality is strict: exact tangency is not reported as penetration. When requested, the
function also returns $p$, $\delta$ and $d$. Those values do not prescribe a response; they are
geometric data that different callers interpret differently.

Spawn clearance uses the same primitive with an enlarged radius. If a candidate has nominal
radius $r$ and requested clearance $c_0$, then it is rejected when

~~~math
d<r+c_0.
~~~

The shared primitive therefore illustrates the architecture directly: **contact geometry can be
reused without forcing the same state response.**

## 3. Head collision: discrete proposal, continuous test

The logical head lives on the integer lattice. If the current state is

~~~math
q\in\mathbb Z^2,
~~~

a movement tick proposes

~~~math
q'=q+d,
\qquad
d\in\{(-1,0),(1,0),(0,-1),(0,1)\}.
~~~

The collision system does not test the integer pair itself. With `TILE_SIZE = 6`, it maps the
proposal to the world-space center

~~~math
H=\Phi(q')=(6q_x'+3,6q_y'+3).
~~~

The gameplay head radius is fixed independently of the selected skin:

~~~math
r_h=8\cdot0.67=5.36.
~~~

The proposed move is rejected if any of the room-edge, wall or self-collision predicates
succeeds. Only after all three fail does `snakeMotionUpdate()` store $q'$ as the new logical
head position.

This ordering gives the head a clear semantics:

~~~math
q
\xrightarrow{\text{proposal}}
q'
\xrightarrow{\Phi}
H
\xrightarrow{D_{\mathrm{head}}}
\begin{cases}
\text{game over}, & D_{\mathrm{head}}=1,\\
q\leftarrow q', & D_{\mathrm{head}}=0.
\end{cases}
~~~

### 3.1 Room edges and doors

Suppose the current room is

~~~math
[x_L,x_R]\times[y_T,y_B]
~~~

with boundary thickness $4$. The left boundary is entered when

~~~math
H_x<x_L+4,
~~~

and the other sides are handled symmetrically.

A side is divided into eight equal segments. The two door segments are indices 3 and 4, so the
local opening occupies

~~~math
[96,160].
~~~

The head center is accepted through that opening when its coordinate $s$ along the side lies in

~~~math
96-r_h\leq s\leq160+r_h,
~~~

and the adjacent room also satisfies the traversal rule. Thus a door is simultaneously a
geometric opening and a progression condition.

Opening a door removes the corresponding two boundary rectangles from `wallState`. The
remaining neighbouring rectangles continue to act as door posts, so the head circle cannot clip
through the side merely because its center lies near the interval.

### 3.2 Head against wall rectangles

[`collidesWithWall()`](../src/collision.c) applies the circle--rectangle predicate to every active
wall. The 32 room-boundary entries use their stored rectangles. Obstacles appended after those
entries are first inset by `WALL_DRAW_INSET = 1`, matching the rectangle that is visibly drawn.

This is an interaction-specific representation. A source `Wall` is the authoritative stored
geometry, but the head/obstacle predicate uses the one-pixel inset copy. The body-joint and
collectible wall interactions do not make the same substitution.

## 4. Self-collision: a thick centerline approximated by samples

The self-collision model is easiest to understand by separating the **continuous geometric
problem** from the **discrete test actually implemented**.

Let

~~~math
J_0,J_1,\ldots,J_{m-1}\in\mathbb R^2
~~~

be the current body chain. For one centerline segment

~~~math
E_i=[J_{i-1},J_i],
~~~

write $d(H,E_i)$ for the Euclidean distance from the candidate head center $H$ to that segment.
A natural continuous model for a body of local half-width $w_i$ would test

~~~math
d(H,E_i)<r_h+w_i.
~~~

Geometrically, this is collision between the head center and the capsule obtained by thickening
$E_i$ by radius $r_h+w_i$.

The current code does **not** compute the exact point-to-segment distance. Instead it samples
five points:

~~~math
\begin{aligned}
t_k&=\frac{k}{4}, \qquad k=0,1,2,3,4,\\
S_{i,k}&=(1-t_k)J_{i-1}+t_kJ_i.
\end{aligned}
~~~

The implemented predicate satisfies

~~~math
D_{\mathrm{self}}(H,E_i)=1
~~~

exactly when there exists $k\in\{0,1,2,3,4\}$ such that

~~~math
\lVert H-S_{i,k}\rVert<r_h+w_i.
~~~

Two roles must not be conflated:

~~~math
\boxed{\text{sampling discretizes the segment; }r_h+w_i\text{ gives the test its thickness}.}
~~~

Testing only the joints would leave the interior of a long link represented only indirectly.
The additional three interior samples reduce that gap by placing overlapping collision
neighbourhoods along the centerline.

If the segment length is

~~~math
L_i=\lVert J_i-J_{i-1}\rVert,
~~~

then consecutive samples are separated by

~~~math
\frac{L_i}{4}.
~~~

`followPreviousJoint()` normally keeps neighbouring joints on the scale of `TILE_SIZE = 6`,
whereas the effective collision radius is $r_h+w_i$, typically much larger than the sample
spacing. This explains why the five-circle approximation is dense in ordinary configurations.
It is not a proof for every possible deformed chain, and the code does not claim one.

The implemented loop considers

~~~math
4\leq i\leq m-2.
~~~

Hence it starts with segment $[J_3,J_4]$ and stops before the final segment
$[J_{m-2},J_{m-1}]$. The effect of the first exclusions is that the proposed head does not test
the nearby neck segments. The final exclusion is documented here only as an implementation
property; the code does not state a separate mathematical reason for it.

If any sampled neighbourhood contains $H$, the proposed logical head step is rejected and the
run ends. The joint chain itself is not modified by this self-collision test.

## 5. Joint constraints: non-penetration without momentum

After an accepted head movement, `bodyChainUpdate()` updates the continuous chain. The relevant
order is

~~~math
\text{head pin}
\longrightarrow
\text{link following}
\longrightarrow
\text{joint separation}
\longrightarrow
\text{wall correction}
\longrightarrow
\text{head re-pin}
\longrightarrow
\text{tangent reconstruction}.
~~~

The head joint is therefore a boundary condition imposed by the logical state:

~~~math
J_0=\Phi(q).
~~~

The remaining operations change positions directly. No joint stores a velocity, so these are
constraint relaxations rather than collision impulses.

### 5.1 Joint--joint separation

For two joints with indices at least 1 and

~~~math
\lvert i-j\rvert>2,
~~~

define

~~~math
d_{ij}=\lVert J_i-J_j\rVert,
\qquad
D_{ij}=0.8(w_i+w_j).
~~~

The admissible separation condition is

~~~math
d_{ij}\geq D_{ij}.
~~~

When

~~~math
10^{-4}<d_{ij}<D_{ij},
~~~

the penetration amount is

~~~math
p_{ij}=D_{ij}-d_{ij}
~~~

and the unit separation direction is

~~~math
n_{ij}=\frac{J_i-J_j}{d_{ij}}.
~~~

The pair correction is symmetric:

~~~math
J_i\leftarrow J_i+\frac{p_{ij}}2n_{ij},
\qquad
J_j\leftarrow J_j-\frac{p_{ij}}2n_{ij}.
~~~

Part of that correction is propagated to as many as two later joints on each branch. For offset
$k=1,2$, the propagated factor is

~~~math
f_k=\frac{0.5}{k}.
~~~

Because the updates are applied in place, a correction made early in the loop changes the data
seen by later comparisons. The nested loop can also encounter the same unordered pair again
from the opposite outer index if it remains within the threshold. The implementation is
therefore a **single sequential relaxation pass**, not a simultaneous solution of a global
constraint system and not an iterative solver run to convergence.

Pairs with distance at or below $10^{-4}$ are skipped because the routine does not construct a
normal from an effectively zero displacement.

### 5.2 Joint--wall correction

For each $i\geq1$, the joint center is tested as a circle of radius $w_i$ against every active
stored wall rectangle. Here the complete stored rectangle is used; the one-pixel inset specific
to head/obstacle collision is not applied.

If the closest point is $p$ and

~~~math
d=\lVert J_i-p\rVert>10^{-4},
~~~

the outward normal is

~~~math
n=\frac{J_i-p}{d}.
~~~

The pass uses half the radial penetration,

~~~math
\beta=\frac{w_i-d}{2},
~~~

and updates

~~~math
J_i\leftarrow J_i+\beta n.
~~~

The previous joint receives $0.5\beta n$ when $i>1$, while as many as three following joints
receive factors $1.5$, $1.0$ and $0.5$. If the closest-point displacement is effectively zero,
the code chooses an axis from the wall dimensions and places the joint center just outside the
corresponding edge.

Again, no velocity is reflected. What looks visually like the body pushing away from a wall is
a positional correction propagated through part of the chain.

## 6. The collectible: overlap, impulse and true velocity reflection

After spawn, the collectible has continuous state

~~~math
(C,v,m),
\qquad
C=(x,y),
\quad
v=(v_x,v_y),
~~~

with $m$ indicating whether it is moving. This makes its response model fundamentally different
from the joint chain: a velocity exists and can be changed by contact.

### 6.1 Spawn clearance is a placement predicate

A room-local candidate is tested against walls with nominal radius

~~~math
r_{\mathrm{spawn}}=
\begin{cases}
7, & \text{apple},\\
16, & \text{coin},
\end{cases}
~~~

plus two pixels of additional clearance. It is also rejected when its center lies within

~~~math
6\sqrt{1.5}
~~~

of any joint center. HUD areas, protected corners and the coin-specific outer margin are filtered
separately.

These rules only choose an initial center. They do not define the collectible's later collision
body.

### 6.2 Head collection is circle--circle overlap

After a head step is accepted, collection occurs when

~~~math
\lVert H-C\rVert<r_h+7.
~~~

The runtime collection radius is 7 for both apples and coins. The event is checked on snake
movement ticks. No collision normal or post-contact velocity is needed because the semantic
result is collection, not bounce.

### 6.3 Body push uses side-profile contact

A stationary collectible can be pushed by the body. For

~~~math
2\leq i\leq m-2,
~~~

the code constructs the two side-profile points

~~~math
P_i^{\pm}=J_i+w_i
\begin{pmatrix}
\cos(\theta_i\pm\pi/2)\\
\sin(\theta_i\pm\pi/2)
\end{pmatrix}.
~~~

These are the same local offsets used to construct the visual profile, but the B-spline itself
is still not tested. Contact is detected when

~~~math
\lVert C-P_i^{\pm}\rVert<0.67w_i+7.
~~~

The first detected contact assigns a velocity of magnitude `COLLISION_FORCE = 1` along the
outward direction. If the centers are effectively coincident, the local side normal from the
joint angle is used.

This is an impulse-like gameplay rule, but it is unilateral: the collectible receives velocity
and the joint chain receives no equal-and-opposite momentum update.

### 6.4 Damped motion

While moving, the collectible first performs

~~~math
C\leftarrow C+v,
\qquad
v\leftarrow0.95v.
~~~

If both components later satisfy

~~~math
\lvert v_x\rvert<0.1,
\qquad
\lvert v_y\rvert<0.1,
~~~

motion is stopped and $v$ is set exactly to zero.

### 6.5 Room-bound bounce

The collectible is confined to the current room using runtime radius 7. If it crosses one of the
four bounds, its center is clamped back inside and the corresponding velocity component is
reversed and damped. For example,

~~~math
v_x\leftarrow-0.8v_x.
~~~

This rule does not inspect door state. An open door can therefore be traversable by the snake
while the collectible remains confined to the room.

### 6.6 Wall reflection

For a wall contact with outward unit normal $n$, the center is first placed at the collision
radius from the closest wall point:

~~~math
C\leftarrow p+7n.
~~~

The velocity is then reflected across the wall normal and damped:

~~~math
v\leftarrow0.8\bigl(v-2(v\cdot n)n\bigr).
~~~

The factor $0.8$ is an implementation damping coefficient; it is not presented as a measured
physical restitution constant. When the closest-point displacement cannot determine a normal,
the code chooses an axis from the wall dimensions before applying the same reflection formula.

This is the collision response in Slither Slide that most closely resembles an ordinary
particle bounce because both position and velocity are updated.

## 7. Interaction-specific radii

There is no single number that should be called "the radius of an object" independently of the
question being asked. The code uses radii attached to interactions:

| Interaction | Effective radius or distance |
| --- | ---: |
| gameplay head | $8\cdot0.67=5.36$ |
| head against body sample | $r_h+w_i$ |
| joint--joint minimum separation | $0.8(w_i+w_j)$ |
| joint against wall | $w_i$ |
| apple spawn clearance | $7+2$ wall clearance |
| coin spawn clearance | $16+2$ wall clearance |
| head collection of either item | $r_h+7$ |
| body-profile push | $0.67w_i+7$ |
| collectible runtime wall/room collision | $7$ |

The coin therefore has radius 16 for spawn clearance and rendering, but the current runtime
collection and bounce rules use radius 7. This is an asymmetry of the implementation, not a
claim that one of those values is the object's unique physical size.

## 8. Frame order is part of the model

The interactions are not only geometrically different; they are evaluated at different moments.
For an active frame, the relevant order is

~~~math
S_n
\xrightarrow{F_{\mathrm{head}}}
S_n^{(1)}
\xrightarrow{F_{\mathrm{chain}}}
S_n^{(2)}
\xrightarrow{F_{\mathrm{collectible}}}
S_{n+1},
~~~

where $F_{\mathrm{head}}$ may terminate the run.

More concretely:

1. on a movement tick, propose the new logical head and test room, walls and current body chain;
2. if accepted, store the new logical head, process transition and collection, and move the tail;
3. later in the frame, pin and relax the continuous joint chain;
4. update collectible motion and wall response;
5. finally allow the corrected body profile to push a stationary collectible.

Thus self-collision compares the **candidate new head** with the body chain as it exists before
that frame's chain update. Joint separation and wall correction occur only after the logical
movement has been accepted.

## 9. Rendering is downstream from collision

The B-spline is built after the world-space joints are projected through the camera. It is a
richer visual surface than any collider used by the gameplay rules, but it is not the
authoritative collision geometry.

This separation has a concrete consequence:

~~~math
\text{render sampling density}\not\Rightarrow\text{gameplay collision density}.
~~~

Changing B-spline samples can change the rasterized outline without silently changing
self-collision. Conversely, changing the five self-collision samples would alter gameplay
collision without changing the mathematical B-spline.

The architecture therefore separates four notions that are easy to conflate:

~~~math
\boxed{
\text{gameplay detection}
\neq
\text{constraint geometry}
\neq
\text{simple object physics}
\neq
\text{render geometry}.
}
~~~

## 10. Limits and validation

The current host tests verify several collision-adjacent invariants: the historical head radius,
selected obstacle clearances, door progression, the final head anchor and reconstructed joint
tangents. They also verify the B-spline independently of the collider.

They do not currently isolate every response described above. In particular, there are no
dedicated host tests that exhaustively characterize:

- the five-sample self-collision approximation;
- joint--joint separation and its propagated correction;
- joint--wall propagation;
- body-to-collectible velocity assignment;
- collectible room and wall reflections.

The formulas in this document are therefore a derivation of the current implementation, not a
formal proof of stability or collision completeness for every possible body configuration.

The main modeling principle can nevertheless be stated precisely: **each interaction uses the
simplest state and geometry that the corresponding game rule actually consumes.**