# Mathematics of the procedural body

Slither Slide uses two descriptions of the snake at the same time:

- a global tile-lattice state for movement, doors and game rules;
- a world-space floating-point joint chain for the animated body.

The B-spline belongs to the second description. At runtime the joint chain is projected to
screen coordinates before the spline is built. It changes the appearance of the body without
changing the logical path of the head. The surrounding room, wall, camera and collectible
representations are derived in [How Slither Slide represents space](spatial-model.md).

## 1. Joint chain

Let

~~~math
J_i=(x_i,y_i),\qquad i=0,\ldots,m-1,
~~~

be the ordered world-space joint centers, with $J_0$ at the head. If the logical head occupies
global snake cell $(q_x,q_y)$ and the tile size is $T$, `bodyChainUpdate()` imposes

~~~math
J_0=\left(Tq_x+\frac{T}{2},\ Tq_y+\frac{T}{2}\right).
~~~

The head is pinned both before and after the constraint pass. It is therefore a boundary condition, not a free point that can drift while the other joints are corrected.

For a following joint, define

~~~math
\Delta_i=J_{i-1}-J_i,\qquad d_i=\|\Delta_i\|.
~~~

When $d_i>T$, the update is

~~~math
J_i'=J_i+\frac{d_i-T}{d_i}\Delta_i,
~~~

which returns the link to length $T$. Otherwise the code uses a softer correction toward $0.9T$:

~~~math
J_i'=J_i+\frac12\frac{d_i-0.9T}{d_i}\Delta_i.
~~~

The normalization is skipped for $d_i\leq10^{-4}$. This avoids dividing by a negligible distance.

The chain then receives two local corrections:

- non-neighbouring joints are separated when their bodies overlap;
- joints inside the clearance of a wall rectangle are pushed outward.

These are position corrections chosen for the scale of the cartridge. They are not an elastic-body simulation. After the corrections, every tangent angle is recomputed from the final joint positions:

~~~math
\theta_i=\mathrm{atan2}(y_{i-1}-y_i,\ x_{i-1}-x_i).
~~~

For the head, the same formula uses $J_0-J_1$.

## 2. From centerline to outline

For local angle $\theta_i$, define the tangent and normal

~~~math
\mathbf t_i=(\cos\theta_i,\sin\theta_i),\qquad
\mathbf n_i=(-\sin\theta_i,\cos\theta_i).
~~~

If $w_i$ is the local half-width, the two sides of the body are

~~~math
L_i=J_i+w_i\mathbf n_i,\qquad
R_i=J_i-w_i\mathbf n_i.
~~~

`snakeBodyWidth()` uses 8 pixels at the head, 7 at the next joint and then a gradual taper from 6 pixels to a minimum of 4.

For the mathematical construction, `snakeGeometryBuild()` turns these offset points into one
ordered control sequence. In the runtime drawing path, `snake_char.c` first translates the
world joints to screen coordinates and supplies those projected joints to this construction.
Let

~~~math
F=J_0+w_0\mathbf t_0
~~~

be the additional point in front of the head. With $m$ joints, the geometric controls $P_0,\ldots,P_{2m}$ are ordered as

~~~math
(P_0,\ldots,P_{2m})=
\bigl(L_{m-1},\ldots,L_0,F,R_0,\ldots,R_{m-1}\bigr).
~~~

Thus the outline is traversed in the order

~~~math
L_{m-1},\ldots,L_0,\ F,\ R_0,\ldots,R_{m-1},
~~~

from the tail to the head along the left side and back to the tail along the right side. Periodic indexing then connects the final right-tail control to the initial left-tail control.

The $P_j$ are therefore not a third geometric construction. They are the offset points $L_i$ and $R_i$, plus the front control $F$, arranged in outline order. The joint index $i$ follows the centerline from head to tail, whereas the control index $j$ follows the closed outline.

Implementation note: `appendControlPoint()` currently converts each screen-space control
coordinate to an integer before spline evaluation. The B-spline samples are still computed in
floating point, and `snake_char.c` converts those samples to integers when calling the RIVES
drawing functions. This pixel-grid conversion is an implementation detail, not part of the
mathematical definition or of the control ordering above. Because the camera projection is a
translation, it preserves the ideal spline shape; truncation is the only raster-specific step
introduced before evaluation.

## 3. Cubic uniform B-spline

A B-spline of degree $p$ is globally defined by all its control points:

~~~math
C(u)=\sum_{j=0}^{n-1}N_{j,p}(u)P_j.
~~~

Its basis functions have local support: in the interior of any knot span, only $p+1$ consecutive basis functions are non-zero. Here $p=3$, so evaluating one span requires four neighbouring controls. The next span shifts this window by one control, and periodic indexing makes the final windows wrap around the closed polygon. Evaluating four controls at a time is therefore the local evaluation of the full spline, not a partition into independent groups of four.

For Slither Slide's cubic uniform basis, let $t\in[0,1)$ be the local parameter of a span:

~~~math
\begin{aligned}
b_0(t)&=\frac{(1-t)^3}{6},\\
b_1(t)&=\frac{3t^3-6t^2+4}{6},\\
b_2(t)&=\frac{-3t^3+3t^2+3t+1}{6},\\
b_3(t)&=\frac{t^3}{6}.
\end{aligned}
~~~

In the spline formulas, $P_j$ always denotes an element of this ordered outline sequence, not a center-chain joint $J_i$. The span beginning at control $P_i$ is

~~~math
C_i(t)=\sum_{k=0}^{3}b_k(t)P_{i+k}.
~~~

The same weighted sum is evaluated for the $x$ and $y$ coordinates. The four weights are non-negative and satisfy

~~~math
\sum_{k=0}^{3}b_k(t)=1.
~~~

Every sample is therefore a convex combination of four nearby controls. This gives the construction three useful properties:

- local support: changing one control affects only neighbouring spans;
- affine invariance: translating, rotating or uniformly scaling the polygon produces the same transformation of the curve;
- $C^2$ continuity between ordinary uniform cubic spans.

The curve approximates the polygon; it is not forced through every control point.

### Midpoint sample

The renderer uses $t=0$ and $t=\tfrac12$ on every span. At the midpoint,

~~~math
b_0\!\left(\frac12\right)=b_3\!\left(\frac12\right)=\frac1{48},
\qquad
b_1\!\left(\frac12\right)=b_2\!\left(\frac12\right)=\frac{23}{48}.
~~~

Hence

~~~math
C_i\!\left(\frac12\right)
=\frac{P_i+23P_{i+1}+23P_{i+2}+P_{i+3}}{48}.
~~~

The two middle controls dominate the sample, while the outer pair provides a smaller symmetric correction.

At a span boundary,

~~~math
\begin{aligned}
C_i'(1)&=\frac{-P_{i+1}+P_{i+3}}2=C_{i+1}'(0),\\
C_i''(1)&=P_{i+1}-2P_{i+2}+P_{i+3}=C_{i+1}''(0).
\end{aligned}
~~~

This is the direct $C^2$ continuity check for the basis used in the code.

## 4. Closure and rasterization

The control indices are taken modulo the number of controls. The final spans reuse the first controls and close the curve without a separate endpoint case.

Each span is sampled at

~~~math
t=0,\quad t=0.5.
~~~

$t=1$ is omitted because it is the $t=0$ sample of the next span. Two samples per span were chosen as a compromise between a smooth outline and the number of RIVES drawing operations.

`snake_char.c` pairs samples from the two sides of the body and fills the strip with triangles. The visible outline is therefore a polygonal raster approximation of the mathematical B-spline.

Collision remains in world coordinates and uses a cheaper description:

- a circle for the head against room boundaries and walls;
- five points on each relevant centerline segment for self-collision.

The collision surface and the rendered surface are close but are not identical.

## 5. Runtime Technical view

![Runtime Technical view over the live snake geometry](media/slither-slide-technical-view.png)

The Technical view displays the complete construction used in the current frame. It projects
the world-space joints with the live camera and reads the already projected `SnakeGeometry`
used by the renderer:

~~~math
\text{joints}\longrightarrow\text{offset controls}
\longrightarrow\text{B-spline samples}\longrightarrow\text{triangles}.
~~~

- pink points: center-chain joints;
- light-blue points and blue lines: complete closed control polygon;
- four orange rings: controls of the selected span;
- yellow points: samples used by the renderer;
- white point: $C_i(t)$ moving through the selected span.

![Selected B-spline span and basis weights](media/slither-slide-technical-view-detail.png)

The four orange controls form a sliding window, not the full control set. A/F or L2/R2 moves that window by one control point with wrap-around. Before manual input, the selected span advances automatically.

The panel reports $t$ and the four current basis weights. The animated white point is explanatory; it is not an extra sample used for the body fill.

The view reads the same `SnakeGeometry` object as the renderer. Its API receives const geometry and joint data, so it cannot modify movement, timers, score, RNG, collision or outcard state.

## 6. Design choices

| Choice | Effect | Reason |
| --- | --- | --- |
| global grid head and world-space floating-point joints | separates rules from appearance | predictable input with curved motion |
| hard extension and soft compression | limits link length without a rigid chain | stable following |
| normal offsets $J_i\pm w_i\mathbf n_i$ | turns a centerline into a profile | body follows local orientation |
| tapered $w_i$ | narrows the profile toward the tail | readable character shape |
| cubic uniform B-spline | local $C^2$ approximation | small fixed basis |
| periodic indices | closes the outline | no endpoint branch |
| two samples per span | polygonal approximation | bounded drawing cost |
| simpler collision primitives | approximate collision | rules do not depend on rendering density |

## 7. Development and sources

Paolo De Marinis selected the B-spline family and configured its use in the cartridge: outline construction, degree and basis, periodic closure and sampling. The implementation was produced with Cursor assistance, integrated into the game and validated by him.

Mathematical background:

- [Dario Andrea Bini, teaching notes index](https://people.dm.unipi.it/bini/Didattica/IAN/appunti/appunti.html)
- [Interpolazione polinomiale a tratti](https://people.dm.unipi.it/bini/Didattica/IAN/appunti/splinenuovo.pdf)
- [Traccia della terza parte del corso di IAN](https://people.dm.unipi.it/bini/Didattica/IAN/appunti/terzaparte.pdf)

Bini's notes cover broader interpolating-spline theory. Slither Slide directly evaluates a closed parametric uniform B-spline and does not solve a global interpolation system.

Procedural-animation study source used during development:

- [Programming Procedural Animations](https://www.youtube.com/watch?v=wFqSKHLb0lo), Programming Chaos.

Retrospective comparison:

- [animal-proc-anim](https://github.com/argonautcode/animal-proc-anim), © 2024 argonaut, MIT License.

The comparison project shares the general idea of a linked center chain, angle-based side offsets and a tapered outline. It was not the source used during the original cartridge development, and no code-lineage claim is made from that similarity. Slither Slide adds its own grid-driven head, constraint rules, periodic cubic B-spline, RIVES rendering and collision model.

## 8. Limits and checks

- Buffers have fixed capacity derived from `MAX_JOINTS`.
- Controls are converted to integer coordinates before sampling.
- The renderer uses only two samples per span.
- Triangle pairing assumes matching order on the two sides.
- Collision does not test the filled triangles.

The host suite checks basis values and partition of unity, exact midpoint weights, control and sample counts, periodic closure, agreement between stored samples and direct evaluation, the final head anchor and tangent reconstruction. These checks cover the implementation invariants used by the cartridge; they are not a general stability proof for every possible joint configuration.
