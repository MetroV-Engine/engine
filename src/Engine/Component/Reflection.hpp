#pragma once

/**
 * @file
 * @brief Reflection of the engine's value types, for the components that
 *        hold them: the editor shows and edits their fields, the level files
 *        save them.
 *
 * Each component is reflected in its own header, under its fields, with
 * E_REFLECT_AS and its short name ("RigidBody"). Only the data a designer
 * sets is reflected: what the systems compute again every step (forces,
 * contacts, a character's State) is left out, so it is neither saved in a
 * level nor edited by hand. A std::variant (Collider::Shape) is reflected
 * as a Variant: the editor shows which shape it holds.
 *
 * Here, outside Engine/Common, so that the math stays free of the ECS.
 */

#include "ECS/Reflection/Reflection.hpp"
#include "Engine/Common/Math/Vector.hpp"
#include "Engine/Common/Shapes/Polygon.hpp"

E_REFLECT_AS(Engine::Math::Vector2f, "Vector2f")
    E_FIELD(x, float)
    E_FIELD(y, float)
E_END

// Only the first `count` vertices are used; see Shapes::Polygon's invariant.
E_REFLECT_AS(Engine::Shapes::Polygon, "Polygon")
    E_FIELD(vertices, std::array<Engine::Math::Vector2f, Engine::Shapes::Polygon::MaxVertices>)
    E_FIELD(count, std::size_t)
E_END
