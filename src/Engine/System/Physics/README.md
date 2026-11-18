# Physics

The engine's 2D physics: forces, integration, collisions and characters. Each
system lives in its own folder; how it works inside is documented in its
header, as docstrings.

## Installing it

`Physics::install` ([Physics.hpp](Physics.hpp)) registers every system in the
order the physics needs. Call `SystemManager::update` with a fixed step,
`Physics::Step` (1/60 s): jump heights, resting speeds and the stability of
springs depend on it.

```cpp
systems.addSystem<PlayerInputSystem>(world);  // the game's systems that write inputs and forces
Physics::install(world, systems);
systems.addSystem<CameraSystem>(world);       // the game's systems that read the results

systems.update(world, Physics::Step);
```

Each system sets the resources it needs (`Resource::GravitySettings`,
`Resource::CollisionSettings`, `Resource::Contacts`) when they are not there
yet. Set your own before `install` to change them.

## Systems, in order

| # | System | Folder | Does |
| --- | --- | --- | --- |
| 1 | `GravitySystem` | [Gravity/](Gravity/) | uniform gravity and `GravityAttractor`s |
| 2 | `SpringSystem` | [Spring/](Spring/) | `Spring` forces between bodies or to an anchor |
| 3 | `RadialFieldSystem` | [Radial/](Radial/) | `RadialField` pushes and pulls |
| 4 | `DragSystem` | [Drag/](Drag/) | air and fluid drag |
| 5 | `BuoyancySystem` | [Buoyancy/](Buoyancy/) | `Buoyant` bodies in `FluidVolume`s |
| 6 | `CharacterControllerSystem` | [Character/](Character/) | runs and jumps the characters, after the forces it compensates |
| 7 | `FrictionSystem` | [Friction/](Friction/) | friction along `SurfaceContact`s, after every other force |
| 8 | `IntegrationSystem` | [Integration/](Integration/) | forces into velocities, velocities into motion |
| 9 | `CollisionSystem` | [Collision/](Collision/) | finds overlapping colliders (sort and sweep), filtered by layer and mask |
| 10 | `CollisionResponseSystem` | [Collision/](Collision/) | separation, bounces, impact friction, and the `SurfaceContact`s the next step reads |

Systems 1 to 5 only add to `RigidBody::force` and `torque`.

## Where the rest lives

| What | Where |
| --- | --- |
| Components (`RigidBody`, `Collider`, `CharacterController`, ...) | [Component/Physics/](../../Component/Physics/) |
| Resources: world-wide data, set once per world (`GravitySettings`, `CollisionSettings`, the step's `Contacts`) | [Resource/Physics/](../../Resource/Physics/) |
| Formulas with no ECS (kinematics, impacts, forces, collision shapes, raycasts) | [Common/Physic/](../../Common/Physic/) |
| Tests | [tests/Engine/System/Physics/](../../../../tests/Engine/System/Physics/) |

## Guides

- [Character](Character/README.md): the player's physics, from the
  components to the tuning, the layers and the known limits.
