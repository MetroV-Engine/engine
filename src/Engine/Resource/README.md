# Resources

ECS resources of the engine: data that belongs to the whole world rather than
to one entity, stored with `world.setResource<T>()` and read with
`world.getResource<T>()`.

Like components, they are plain data with no logic; the systems read and write
them. A value per entity is a component ([Component/](../Component/)); a value
for the whole world, or shared between systems, is a resource.

| File | Namespace | What |
| --- | --- | --- |
| [Physics/GravitySettings.hpp](Physics/GravitySettings.hpp) | `Engine::Resource` | Uniform gravity and the constant G. `GravitySettings::of(world)` reads it, or the defaults |
| [Physics/CollisionSettings.hpp](Physics/CollisionSettings.hpp) | `Engine::Resource` | Settings of the collision response (`restingSpeed`) |
| [Physics/Contacts.hpp](Physics/Contacts.hpp) | `Engine::Resource` | The pairs of overlapping colliders, rebuilt by `CollisionSystem` every step, read by the response, the characters and gameplay |

The systems that need one set its defaults in `init()` when the world has none:
set your own before adding them to change it.
