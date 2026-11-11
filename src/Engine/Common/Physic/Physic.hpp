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
 * - Impact.hpp: bounce (restitution) and separation of two bodies that hit.
 */

#include "Forces/Dynamics.hpp"
#include "Forces/Friction.hpp"
#include "Forces/Gravity.hpp"
#include "Forces/Radial.hpp"
#include "Forces/Spring.hpp"
#include "Forces/Surface.hpp"
#include "Impact.hpp"
#include "Kinematics.hpp"
