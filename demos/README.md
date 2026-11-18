# Demos

Small SDL3 programs to see engine code at work. They are built only when
`ENGINE_BUILD_DEMOS=ON`, which also selects the `gui` vcpkg feature (SDL3 and
Dear ImGui).

## Build

Run from the repository root:

```sh
cmake -S . -B cmake-build-demos -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DENGINE_BUILD_DEMOS=ON \
    -DENGINE_BUILD_TESTS=OFF \
    -DENGINE_BUILD_APP=OFF
cmake --build cmake-build-demos --target demo_collision demo_friction demo_spring demo_showcase
```

Each demo is built only when the engine code it shows exists on the current
branch (see [CMakeLists.txt](CMakeLists.txt)).

## demo_collision

```sh
./cmake-build-demos/demos/demo_collision
```

An interactive test bench for the collision geometry
([src/Engine/Common/Physic/Collision](../src/Engine/Common/Physic/Collision)): move shapes and
see what `collide()`, `raycast()` and `sweep()` return.

| Mode | Shows |
| --- | --- |
| Collide (`1`) | The selected shape against every other one: contact point, normal (from the other shape towards the selected one), depth, and where the shape goes once pushed out. Red: overlapping; yellow: only touching (depth 0). |
| Raycast (`2`) | A ray between two handles: every hit with its normal, the first one circled. A ray that starts inside a shape ignores it. |
| Sweep (`3`) | The selected shape moved by the arrow: where it stops, the normal, and the end of the move tested alone, which misses thin walls on fast moves. |

| Control | Action |
| --- | --- |
| Left drag | Move a shape or a handle; on empty space, pan |
| Right or middle drag | Pan |
| Wheel | Zoom around the cursor |
| `Delete` | Remove the selected shape |
| `G` | Snap to a 0.25 m grid: exact touches and ties, as with tiles |

The panel adds shapes, edits the selected one, and sets a **world offset**: every
query then runs with the scene moved 1 km, 10 km or 100 km from the origin, and
the panel shows how far the result moves compared to the same query at the
origin (see [BenchCollisionRecentering.cpp](../benchmarks/BenchCollisionRecentering.cpp)).

`--screenshot file.bmp [--mode collide|raycast|sweep] [--offset metres]` renders
a few frames, saves the image and quits, for example without a display:

```sh
SDL_VIDEO_DRIVER=offscreen ./cmake-build-demos/demos/demo_collision --screenshot sweep.bmp --mode sweep
```

## demo_friction

```sh
./cmake-build-demos/demos/demo_friction
```

Boxes moved by the engine's own systems (`GravitySystem`, `FrictionSystem`,
`IntegrationSystem`) at a fixed 60 Hz, with what Coulomb's law predicts next to
what they do.

| Scene | Checks |
| --- | --- |
| Slide (`1`) | Three boxes launched at the same speed on ice, wood and rubber stop on their green mark, at v0² / (2 μk g), and never move back. |
| Slope (`2`) | A resting box holds while tan(angle) ≤ μs, and slides at g (sin − μk cos) otherwise (angle and coefficients in the panel). |
| Conveyor (`3`) | A box put on a moving belt is dragged at μk g up to the belt's speed, then carried at it. |
| Push (`4`) | Hold `Space` to push a 10 kg crate: it holds while the push is at most μs N, then accelerates at (F − μk N) / m. |

`P` pauses, `R` resets, and the panel steps 1/60 s at a time or slows time down.
The ground's support is not part of `FrictionSystem` (it will come from the
collision response): the demo stands in for it with its own `SupportSystem`,
which puts each box back on its ground after every step.

`--screenshot file.bmp [--scene slide|slope|conveyor|push] [--seconds s] [--push newtons]`
runs the scene for `s` seconds, saves the image and quits.

## demo_spring

```sh
./cmake-build-demos/demos/demo_spring
```

Bodies on springs moved by the engine's own systems (`GravitySystem`,
`SpringSystem`, `IntegrationSystem`) at a fixed 60 Hz, with what Hooke's law
predicts next to what they do. Springs are orange when stretched, blue when
compressed.

| Scene | Checks |
| --- | --- |
| Oscillators (`1`) | Four weights hung from hooks (entities without `RigidBody`), damping ratios 0, 0.25, 1 and 2: equilibrium at L0 + m g / k, period 2π / (ω0 √(1 − ζ²)) next to what semi-implicit Euler gives at 60 Hz, no overshoot from ζ = 1. Raise k: each one blows up past dt ω0 = 2 (√(1 + ζ²) − ζ), the overdamped one first. |
| Rope (`2`) | A 70 kg weight on a stiff spring to a fixed point swings like a pendulum: period 2π √(L / g) (1 + a² / 16), tension at the bottom m g (3 − 2 cos a), and the swing keeps its amplitude, since the damping acts along the spring only. |
| Bridge (`3`) | Links joined by springs between two hooks: each link has two springs, and the hooks carry the whole weight. Right click a spring to cut it; remove a hook and its spring breaks. |
| Pair (`4`) | Two bodies joined by a spring, in space: the momentum stays the same and the center of mass moves in a straight line. |

In every scene, drag a body with the left button: the demo pulls it with a
spring of its own. In the Rope scene, click elsewhere to hook the rope there,
`Space` lets go and `A` / `D` push. `P` pauses, `R` resets.

The bridge starts at rest in the shape it hangs in (the demo lets it settle with
extra damping first): dropped straight, it bounces up and down for about a
minute, because the damping acts along the springs only.

`--screenshot file.bmp [--scene oscillators|rope|bridge|pair] [--seconds s] [--stiffness k]`
runs the scene for `s` seconds, saves the image and quits.

## demo_showcase

```sh
./cmake-build-demos/demos/demo_showcase
```

Every physics feature of the engine in one level, with a character to walk
through it. Everything that moves is moved by the engine: the demo calls
`Physics::install` once and `SystemManager::update(world, Physics::Step)` at a
fixed 60 Hz. The demo itself only draws (`Collider::worldShape`, the
`Transform` rotation, the `Contacts` resource for the debug drawing), turns the
keyboard into a `CharacterInput`, and builds the level out of components. Two
small level scripts set velocities: the platform's back and forth, and `T`
dropping the bodies again.

| Zone | Shows |
| --- | --- |
| 1 Crates | Boxes, polygons, a pyramid; a plank that tips over (rotation); a 400 kg crate whose static friction is above the character's `pushStrength`. Left of the start, a pink monster on layer `Enemy`: the hero's mask leaves it out, so the hero walks through it, while the ground and the crate on its head still collide with it (`Collider::layer` / `mask`, toggle in the panel). |
| 2 Bounce | Balls of restitution 0.9, 0.6 and 0.2 dropped from 6 m: the panel measures e = sqrt(h2 / h1) from successive apexes. |
| 3 Slopes | A hill of `Collider::Polygon::slope`; a crate held on 26.6° by static friction; an icy crate that slides down, across the ice, and stops on the rubber; a ball and an octagon to roll down. On the ice the character's feet slip (`CharacterController::fullGripFriction`): it speeds up slowly and slides on when it stops. |
| 4 Platforms | A kinematic platform (mass 0) over a pit that carries the character; a kinematic bar turning on itself. |
| 5 Springs | A rope over a trench: links joined end to end by `Spring`s held off center (`bodyPoint`, `otherPoint`), so each link turns; a lantern hanging by its top; a ceiling to grapple. Springs stable at 60 Hz are too soft for a taut rope: it sags like a hammock, jump out at its far end. |
| 6 Water | A `FluidVolume`: crates, a ball, a triangle, a raft and the character float with m / (area × ρ) of their area under the surface, the raft levels itself, the stone sinks. |
| 7 Drag | A drop tower: no drag, linear `Drag` (terminal speed m g / c), quadratic in a breeze (sqrt(m g / c)); a fan (`RadialField`) holding two balls up in a tube hung high enough to walk under. |
| 8 Gravity | A `GravityAttractor` planet with two moons in orbit (gravityScale 0). |

| Control | Action |
| --- | --- |
| `Q` or `A` / `D`, arrows | Run |
| `Space`, `Z` or `W`, `Up` | Jump: hold for the full height, release early for less |
| Left drag | Grab any body: a `Spring` from it to the cursor |
| Right click | Grapple: a `Spring` to the scenery hit by `Collision::raycast`, the character handed to physics (`handToPhysics`); `Z` or `W` / `S` reel, `Space` or right click lets go |
| `E` or middle click | Explosion at the cursor: a `RadialField` that lasts one step |
| `B` / `O` / `H` | Drop a crate / a ball / a hexagon at the cursor |
| `T` | Drop the balls of zone 2 and the tower of zone 7 again |
| `1`-`8`, `F1`, `P`, `N`, `R`, wheel | Zones, debug drawing (contacts, velocities, ground normal), pause, one step, reset, zoom |

The panel shows the character's state and tuning, gravity and resting speed,
the time per step, and each zone's measurements next to the theory.

`--selftest` runs the level without a window and checks those measurements,
the character (speed, jump height, ice, steady push, platform, rope) and the
interactions (grapple, grab, explosion); it exits with 1 when one fails. Two
limits of the engine as it is are reported as `KNOWN`, not as failures: the
rope's sag, and a 400 kg crate that creeps (3 cm in 1.5 s) when pushed
steadily with less than its static friction.
`--screenshot file.bmp [--zone 1-8] [--seconds s] [--move -1..1]` runs `s`
seconds holding `--move`, saves the image and quits.

## demo_playground

Built only on the branch with the earlier `Engine/Physics` API it uses.

```sh
./cmake-build-demos/demos/demo_playground
```

A platformer character in a level that uses every physics feature of the
engine, all of it the engine's own: `Physics::install`, a `TileMap` with slopes
and one-way platforms, a `CharacterController`, colliders with materials,
layers and triggers, springs and ropes, fields, water, the collision response.
The panel switches the engine's systems on and off, shows the character's
state, and compares the measurements with what is expected.

| Zone | Checks |
| --- | --- |
| 1 Crates (5 kg, 50 kg) | The character pushes them with a force: the light one slides, the heavy one hardly. |
| 2 Balls dropped from 6 m | Restitution at each bounce: v out / v in gives exactly e = 0.9, 0.6, 0.1. Running into a ball sets it going at the character's speed, no faster. |
| 3 Ice, then rubber | Crates slide on ice (the slipperier material counts), grip on rubber. |
| 4 Conveyor belt | Crates and the character are carried at 3 m/s. |
| 5 Bounce pad (e = 0.9) | Jump onto the one-way ledge from below, walk off it onto the pad. |
| 6 Bridge | Links on springs between two hooks: it bends under the character, who weighs 70 kg on what it stands on. |
| 7 Grapple | Right click: a rope (a `Spring` with `rope = true`) holds where `Physics::raycast` hits; off the ground the character is handed to physics and swings. |
| 8 Gravity well, air vent | A `GravityAttractor` (4.5 m of range) lifts what passes under it, the character too; a `RadialField` under the ground throws bodies and the character up. |
| 9 Slopes | 45 and 22.5 degree hills: the character walks up and down them at its speed, and stands on them. |
| 10 One-way platforms, shaft | Jump up through platforms, drop through them (`S` + `Space`), climb the shaft by wall jumps. |
| 11 Water | A `FluidVolume`: light crates and the character (49 kg/m²) float, the 60 kg crate sinks, the current carries them all. Under water, jump is a stroke; at the surface, a jump out. |
| 12 Moving platform | A kinematic body that carries the character across the pit. |

Coins are trigger colliders: `TriggerEvents` reports them, and they are removed
after the frame's steps (nothing is added or removed during a step).

| Control | Action |
| --- | --- |
| `Q` or `A` / `D` | Move (AZERTY or QWERTY) |
| `Z`, `W` or `Space` | Jump: hold it for a full jump (it peaks exactly at the aimed height), release it early for a lower one. Coming down, the character's gravity is 1.2 times stronger (`fallGravityScale`, in the panel); the world's is unchanged |
| `S` + `Space` | Drop through a one-way platform |
| `Shift` | Dash |
| Right click | Grapple, from the ground too: the rope, 1 m shorter than the distance, lifts the character and swings it; `Z` or `W` / `S` reel, `Q` or `A` / `D` swing, `Space` or right click lets go |
| Left drag | Grab any body with a spring |
| `E` or middle click | Explosion at the cursor (`Physics::applyRadialImpulse`) |
| Wheel | Zoom |
| `1`-`9`, `0`, `F1`, `P`, `R` | Zones, the engine's debug drawing (`Physics::debugDraw`), pause, reset |

`--screenshot file.bmp [--at zone] [--seconds s] [--debug] [--move -1|1] [--jump] [--dash] [--grapple x y] [--explode x y]`
runs a scripted scene for `s` seconds, saves the image and quits.
