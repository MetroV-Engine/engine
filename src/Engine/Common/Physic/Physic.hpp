#pragma once

/**
 * @file
 * @brief Entry point of the engine's physics formulas: include this.
 *
 * - Kinematics.hpp: integration of a step, trajectories, jump height.
 * - Forces/Dynamics.hpp: F = m * a, impulses, static bodies.
 * - Forces/Gravity.hpp: weight (uniform gravity) and universal gravitation.
 * - Forces/Surface.hpp: normal and tangential parts, normal force N.
 * - Forces/Friction.hpp: static and kinetic friction between surfaces.
 * - Forces/Spring.hpp: damped spring (Hooke's law).
 * - Forces/Radial.hpp: radial push (explosions, shockwaves).
 * - Forces/Drag.hpp: air and water resistance.
 * - Forces/Buoyancy.hpp: Archimedes' push in a fluid.
 * - Rotation.hpp: moments of inertia, torque, angular integration.
 * - Impact.hpp: bounce (restitution) and separation of two bodies that hit,
 *   at their center or, with rotation, at the contact point.
 */

#include "Forces/Buoyancy.hpp"
#include "Forces/Drag.hpp"
#include "Forces/Dynamics.hpp"
#include "Forces/Friction.hpp"
#include "Forces/Gravity.hpp"
#include "Forces/Radial.hpp"
#include "Forces/Spring.hpp"
#include "Forces/Surface.hpp"
#include "Impact.hpp"
#include "Kinematics.hpp"
#include "Rotation.hpp"
