# Mathematics of the procedural body

The visible snake is not the logical snake drawn with rounded corners. Slither Slide first keeps
a discrete state for gameplay, derives a continuous centerline from that state, turns the
centerline into a closed profile and only then approximates that profile with a periodic cubic
B-spline.

The construction is therefore best read as a sequence of maps:

~~~math
q
\longrightarrow
J
\longrightarrow
(L,R,F)
\longrightarrow
P
\longrightarrow
C
\longrightarrow
\text{raster triangles}.
~~~

Each arrow answers a different question. The logical head $q$ determines where gameplay says
the snake is. The joint chain $J$ determines a deformable centerline. The offsets $L_i,R_i$
give that centerline width. The ordered controls $P_j$ define one closed B-spline. Sampling the
curve finally gives the finite geometry consumed by the renderer.

The surrounding world and the distinction between discrete and continuous coordinates are
derived in [How Slither Slide represents space](spatial-model.md). Collision uses the same
joint chain but a different geometric approximation, derived in
[How Slither Slide separates collision, constraints and response](collision-model.md).

## 1. From logical head to continuous centerline

Let the ordered world-space joint centers be

~~~math
J=(J_0,\ldots,J_{m-1}),
\qquad
J_i=(x_i,y_i)\in\mathbb R^2,
~~~

with $J_0$ at the head. If the logical head occupies the global snake cell

~~~math
q=(q_x,q_y)
~~~

and the tile size is $T=6$, the first joint is not free:

~~~math
J_0=\Phi(q)=\left(Tq_x+\frac T2,\ Tq_y+\frac T2\right).
~~~

`bodyChainUpdate()` pins this point before and after the constraint pass. The discrete head is
therefore a boundary condition for the continuous chain.

For a following joint, define

~~~math
\Delta_i=J_{i-1}-J_i,
\qquad
d_i=\lVert\Delta_i\rVert.
~~~

The local following operator depends on whether the link is extended beyond $T$. If

~~~math
d_i>T,
~~~

the update is

~~~math
J_i'
=J_i+\frac{d_i-T}{d_i}\Delta_i,
~~~

which moves $J_i$ exactly far enough along the current link direction to return the distance to
$T$.

When

~~~math
10^{-4}<d_i\leq T,
~~~

the code instead uses

~~~math
J_i'
=J_i+\frac12\frac{d_i-0.9T}{d_i}\Delta_i.
~~~

This is a softer correction toward $0.9T$. For $d_i\leq10^{-4}$ the normalization is skipped.

The following rule is only one part of the chain update. Non-neighbouring joints may then be
separated and joints may be pushed away from walls. Those are positional constraints, not
velocity-based dynamics; their exact predicates and responses are derived in the collision
model.

After all positional corrections, tangent angles are reconstructed from the final chain. For
$i\geq1$,

~~~math
\theta_i
=\mathrm{atan2}
(y_{i-1}-y_i,\ x_{i-1}-x_i),
~~~

while the head uses the direction of $J_0-J_1$.

The important dependency is therefore

~~~math
\text{positions first}\longrightarrow\text{tangents afterwards}.
~~~

The tangent is a derived geometric quantity, not an independent degree of freedom.

## 2. From centerline to a body profile

A centerline has no thickness. To obtain a visible body, define at each joint the unit tangent
and its perpendicular normal:

~~~math
\mathbf t_i=(\cos\theta_i,\sin\theta_i),
\qquad
\mathbf n_i=(-\sin\theta_i,\cos\theta_i).
~~~

Let $w_i$ be the local half-width returned by `snakeBodyWidth()`. The two profile points are

~~~math
L_i=J_i+w_i\mathbf n_i,
\qquad
R_i=J_i-w_i\mathbf n_i.
~~~

The width law is discrete in the joint index:

~~~math
w_i=
\begin{cases}
8, & i=0,\\
7, & i=1,\\
\max\left(6-\dfrac{i}{24},4\right), & i\geq2.
\end{cases}
~~~

The body therefore narrows gradually toward the tail after the first two joints.

One additional point extends the profile in front of the head:

~~~math
F=J_0+w_0\mathbf t_0.
~~~

The sets $\{L_i\}$ and $\{R_i\}$ are not yet a closed curve. They must first be ordered around
the boundary.

## 3. The control polygon is one ordered closed outline

With $m$ joints, `snakeGeometryBuild()` creates $2m+1$ controls. Ignoring raster conversion for
the moment, their geometric order is

~~~math
(P_0,\ldots,P_{2m})
=
\bigl(
L_{m-1},\ldots,L_0,
F,
R_0,\ldots,R_{m-1}
\bigr).
~~~

The control index therefore follows the boundary rather than the centerline:

~~~math
\text{tail left}
\longrightarrow
\text{head left}
\longrightarrow
F
\longrightarrow
\text{head right}
\longrightarrow
\text{tail right}.
~~~

This distinction between indices matters. $J_i$ refers to a point on the centerline, while
$P_j$ refers to a point in the ordered outline. The controls are not a third independent body
model; they are the two offset profiles plus $F$ written in boundary order.

Periodic indexing closes the polygon algebraically. There is no separate terminal spline case:
a control index outside $0,\ldots,2m$ is read modulo the total control count.

### Runtime coordinate conversion

The mathematical construction above can be described in world coordinates because translation
preserves its shape. The runtime drawing path performs one extra step first:

~~~math
J_i^{\mathrm{screen}}=J_i-K,
~~~

where $K$ is the camera translation. `snake_char.c` supplies these projected joints to
`snakeGeometryBuild()`.

`appendControlPoint()` then converts every control coordinate toward zero to an integer before
spline evaluation. If $\tau$ denotes componentwise C integer conversion, the actual stored
control sequence is

~~~math
(\widehat P_0,\ldots,\widehat P_{2m})
=
\bigl(
\tau(L_{m-1}),\ldots,\tau(L_0),
\tau(F),
\tau(R_0),\ldots,\tau(R_{m-1})
\bigr)
~~~

in screen coordinates.

The truncation is an implementation-level rasterization effect. It is not part of the ideal
profile definition. The B-spline samples themselves are again accumulated in floating point.

## 4. One cubic B-spline, evaluated locally

A degree-$p$ B-spline is globally determined by an ordered control sequence:

~~~math
C(u)=\sum_j N_{j,p}(u)P_j.
~~~

The defining property relevant here is local support. For degree $p=3$, only four consecutive
basis functions are non-zero inside one span. Therefore evaluating four controls at a time does
**not** create independent four-point splines. It evaluates local spans of one global periodic
curve.

For the uniform cubic basis used by the cartridge, let $t\in[0,1)$ be the local span parameter:

~~~math
\begin{aligned}
b_0(t)&=\frac{(1-t)^3}{6},\\
b_1(t)&=\frac{3t^3-6t^2+4}{6},\\
b_2(t)&=\frac{-3t^3+3t^2+3t+1}{6},\\
b_3(t)&=\frac{t^3}{6}.
\end{aligned}
~~~

For a span beginning at control $P_i$,

~~~math
C_i(t)
=\sum_{k=0}^{3}b_k(t)P_{i+k},
~~~

where control indices are interpreted periodically.

The weights satisfy

~~~math
b_k(t)\geq0,
\qquad
\sum_{k=0}^{3}b_k(t)=1.
~~~

Each point $C_i(t)$ is therefore a convex combination of four nearby controls. This immediately
explains why moving one control changes only neighbouring spans and why the curve approximates,
rather than interpolates, the control polygon.

## 5. Continuity comes from the basis, not from stitching curves

Because consecutive spans use shifted windows of the same uniform basis, they agree through
second derivative at their common boundary.

At $t=1$ for span $i$ and $t=0$ for span $i+1$,

~~~math
C_i'(1)
=\frac{-P_{i+1}+P_{i+3}}2
=C_{i+1}'(0),
~~~

and

~~~math
C_i''(1)
=P_{i+1}-2P_{i+2}+P_{i+3}
=C_{i+1}''(0).
~~~

Thus the ordinary uniform spans join with $C^2$ continuity. Periodic indexing applies the same
local rule at the closure, so the final spans reuse the first controls instead of switching to a
special endpoint construction.

The smoothness is therefore a property of one periodic spline evaluated span by span, not the
result of manually joining separate cubic curves.

## 6. Runtime sampling is a second approximation

The mathematical curve contains infinitely many parameter values. The renderer cannot draw that
object directly, so the implementation samples every span at

~~~math
t=0,
\qquad
t=\frac12.
~~~

Since `SNAKE_SAMPLES_PER_SPAN = 2`, a geometry with $n$ controls produces

~~~math
2n
~~~

stored curve samples. The endpoint $t=1$ is not sampled separately; geometrically it coincides
with the next span's $t=0$ boundary value.

The midpoint weights are

~~~math
b_0\!\left(\frac12\right)
=b_3\!\left(\frac12\right)
=\frac1{48},
~~~

and

~~~math
b_1\!\left(\frac12\right)
=b_2\!\left(\frac12\right)
=\frac{23}{48}.
~~~

Hence

~~~math
C_i\!\left(\frac12\right)
=
\frac{P_i+23P_{i+1}+23P_{i+2}+P_{i+3}}{48}.
~~~

The renderer then uses the finite sample sequence rather than the analytic spline. Samples from
opposite sides of the ordered outline are paired and filled with triangles, and the outline is
drawn as line segments between successive samples.

There are therefore two distinct approximations downstream from the continuous centerline:

~~~math
\text{ideal offset controls}
\xrightarrow{\tau}
\text{integer-valued stored controls}
\xrightarrow{t\in\{0,1/2\}}
\text{finite curve samples}
\xrightarrow{\text{triangles}}
\text{raster body}.
~~~

Neither approximation changes the logical head path.

## 7. Rendering geometry and collision geometry deliberately diverge

The B-spline is the richest representation of the visible body, but collision does not use the
filled triangles as an authoritative surface.

The head is tested against room and wall geometry as a circle. Self-collision instead thickens
selected centerline segments and approximates them with five sample points per segment. Joint
constraints operate directly on $J_i$. The collectible uses still other local contact models.

Thus

~~~math
\boxed{
\text{rendered surface}
\neq
\text{gameplay collider}.
}
~~~

This separation prevents a purely visual parameter such as `SNAKE_SAMPLES_PER_SPAN` from
silently becoming a gameplay parameter. Increasing B-spline sampling density can improve the
picture without changing self-collision, while changing the self-collision samples can change
gameplay without altering the analytic spline.

The exact collision predicates are derived in [the collision model](collision-model.md).

## 8. Runtime Technical view

![Runtime Technical view over the live snake geometry](media/slither-slide-technical-view.png)

The Technical view exposes the same construction used by the renderer:

~~~math
\text{projected joints}
\longrightarrow
\text{offset controls}
\longrightarrow
\text{periodic B-spline samples}
\longrightarrow
\text{triangles}.
~~~

The overlay uses the following visual roles:

- pink points: projected center-chain joints;
- light-blue points and blue lines: the complete closed control polygon;
- four orange rings: the local control window for the selected span;
- yellow points: the samples stored for rendering;
- white point: the explanatory value $C_i(t)$ moving through that span.

![Selected B-spline span and basis weights](media/slither-slide-technical-view-detail.png)

The four orange controls are therefore a local window into the complete polygon, not the full
control set. A/F or L2/R2 changes the selected span by one control with periodic wrap-around.
Before manual selection, the displayed span advances automatically.

The white point is not an additional renderer sample. It is evaluated for explanation while the
actual body continues to use the stored two-samples-per-span geometry.

The view receives const geometry and joint data, so it reads the live construction without
changing movement, score, timers, RNG, collision or outcard state.

## 9. What is mathematical structure and what is an implementation parameter

The construction contains both structural choices and numerical parameters. They should not be
presented with the same status.

The following relations define the current geometric architecture:

~~~math
J_0=\Phi(q),
\qquad
L_i=J_i+w_i\mathbf n_i,
\qquad
R_i=J_i-w_i\mathbf n_i,
~~~

followed by one periodically indexed cubic uniform B-spline over the ordered outline.

Other values belong to the implementation rather than to a general spline theory: the taper law
for $w_i$, integer conversion of controls, two samples per span, fixed buffer capacities and the
specific positional-constraint coefficients used to update $J$.

This distinction matters when interpreting the code. The B-spline basis gives mathematical
properties such as local support and $C^2$ span continuity. Values such as two renderer samples
per span are finite engineering choices and are not consequences of those properties.

## 10. Development, sources and limits

Paolo De Marinis selected the B-spline family and configured its use in the cartridge: the
outline construction, degree and basis, periodic closure and runtime sampling. The implementation
was produced with Cursor assistance, integrated into the game and validated by him.

Mathematical background:

- [Dario Andrea Bini, teaching notes index](https://people.dm.unipi.it/bini/Didattica/IAN/appunti/appunti.html)
- [Interpolazione polinomiale a tratti](https://people.dm.unipi.it/bini/Didattica/IAN/appunti/splinenuovo.pdf)
- [Traccia della terza parte del corso di IAN](https://people.dm.unipi.it/bini/Didattica/IAN/appunti/terzaparte.pdf)

Bini's notes cover broader interpolating-spline theory. Slither Slide directly evaluates a
closed parametric uniform B-spline; it does not solve a global interpolation system.

Procedural-animation study source used during development:

- [Programming Procedural Animations](https://www.youtube.com/watch?v=wFqSKHLb0lo), Programming Chaos.

Retrospective comparison:

- [animal-proc-anim](https://github.com/argonautcode/animal-proc-anim), © 2024 argonaut, MIT License.

That project shares the broad idea of a linked center chain, angle-based side offsets and a
tapered outline. It was not the source used during the original cartridge development, and no
code-lineage claim is made from the similarity.

The current implementation also has explicit limits:

- buffers have fixed capacity derived from `MAX_JOINTS`;
- stored controls are converted to integer coordinates before spline evaluation;
- every span contributes only two renderer samples;
- triangle filling assumes the current periodic sample ordering;
- collision does not test the filled spline surface.

The host suite checks basis values and partition of unity, midpoint weights, control and sample
counts, periodic closure, agreement between stored samples and direct span evaluation, the final
head anchor and tangent reconstruction. These checks establish implementation invariants used by
the cartridge; they are not a general stability proof for every possible joint configuration.