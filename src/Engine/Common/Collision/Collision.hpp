#pragma once

/**
 * @file
 * @brief Entry point of the collision geometry: include this.
 *
 * Pure geometry, independent of the ECS: header-only, no allocation, no
 * state. The collision system and gameplay code (a character controller,
 * a line-of-sight check) both build on it.
 *
 * - Shapes/: what is tested, Rectangle (axis-aligned) and Circle.
 * - Contact.hpp, Hit.hpp: what the tests return. Contact for collide(),
 *   Hit for raycast() and sweep().
 * - Overlap.hpp: contains() and overlaps(), the yes/no tests.
 * - Collide.hpp: collide(), the contact (normal, depth, point) of two
 *   overlapping shapes.
 * - Raycast.hpp: raycast(), the first hit of a ray on a shape.
 * - Sweep.hpp: sweep(), the first contact of a moving shape with a still
 *   one (continuous collision: no tunneling through thin walls).
 *
 * Conventions, the same in every file:
 * - World space, meters, float.
 * - Edges are included: touching counts as overlapping (depth 0).
 * - A normal goes from the first shape towards the second (collide), or
 *   out of the hit surface towards where the motion came from (raycast,
 *   sweep). It always has length 1.
 * - Rays and sweeps ignore shapes they start inside of.
 * - Ties between axes go to Y: a corner hit exactly counts as a floor or a
 *   ceiling, not a wall.
 */

#include "Collide.hpp"
#include "Contact.hpp"
#include "Hit.hpp"
#include "Overlap.hpp"
#include "Raycast.hpp"
#include "Shapes/Circle.hpp"
#include "Shapes/Rectangle.hpp"
#include "Sweep.hpp"
