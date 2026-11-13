#pragma once

/**
 * @file
 * @brief The shapes the collision queries take, under Engine::Collision.
 *
 * The shapes themselves live in Engine/Common/Shapes (namespace
 * Engine::Shapes): they are plain geometry, which other code (rendering,
 * buoyancy) can use without the collision queries. They are declared here
 * in Engine::Collision too, so collision code and its callers name them
 * Collision::Rectangle, Collision::Circle and Collision::Polygon.
 */

#include "../../Math/Vector.hpp"
#include "../../Shapes/Circle.hpp"
#include "../../Shapes/Polygon.hpp"
#include "../../Shapes/Rectangle.hpp"

namespace Engine::Collision {
    using Math::Vector2f;
    using Shapes::Circle;
    using Shapes::Polygon;
    using Shapes::Rectangle;
}
