# Mathematics of the procedural body

Slither Slide uses two descriptions of the snake at the same time:

- a tile-grid state for movement, doors and game rules;
- a floating-point joint chain for the body drawn on screen.

The B-spline belongs to the second description. It changes the appearance of the body without changing the logical path of the head.

## 1. Joint chain

Let

~~~math
J_i=(x_i,y_i),\qquad i=0,\ldots,m-1,
~~~

be the ordered joint centers, with $J_0$ at the head. If the logical head occupies grid cell $(q_x,q_y)$ and the tile size is $T$, `bodyChainUpdate()` imposes

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

`snakeGeometryBuild()` orders the controls as:

1. one side from tail to head;
2. one point in front of the head;
3. the other side from head to tail.

With $m$ joints, the closed polygon contains $2m+1$ controls. The center joints and the B-spline controls are therefore different objects: the controls lie on the offset outline.

## 3. Cubic uniform B-spline

Each span uses four consecutive controls and a parameter $t\in[0,1)$:

~~~math
\begin{aligned}
b_0(t)&=\frac{(1-t)^3}{6},\\
b_1(t)&=\frac{3t^3-6t^2+4}{6},\\
b_2(t)&=\frac{-3t^3+3t^2+3t+1}{6},\\
b_3(t)&=\frac{t^3}{6}.
\end{aligned}
~~~

The span beginning at control $P_i$ is

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

Collision uses a cheaper description:

- a circle for the head against room boundaries and walls;
- five points on each relevant centerline segment for self-collision.

The collision surface and the rendered surface are close but are not identical.

## 5. Runtime Technical view

![Runtime Technical view over the live snake geometry](media/slither-slide-technical-view.png)

The Technical view displays the complete construction used in the current frame:

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
| Grid head and floating-point joints | separates rules from appearance | predictable input with curved motion |
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
