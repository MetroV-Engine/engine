#pragma once

#include <string>

#include "ECS/ECS.hpp"
#include "Engine/Services/Services.hpp"
#include "demo/Components.hpp"
#include "demo/Terminal.hpp"

/**
 * @file
 * @brief The Sokoban's logic, as systems. A level file lists them by name,
 *        in this order: InputSystem, MoveSystem, ProgressSystem, RenderSystem.
 *
 * Registered with E_SYSTEM here, in the header: a system only chosen by its
 * name is referenced by no code, so its registration must be compiled into
 * the executable (see the ECS README).
 */
namespace demo {
    /** @brief Boxes on a goal, out of all the boxes (a resource, written by ProgressSystem). */
    struct Progress {
        int placed = 0;
        int total = 0;
        bool solved() const { return total > 0 && placed == total; }
    };

    namespace detail {
        // The entity at a cell with component T, or an invalid one.
        template<typename T>
        ECS::Entity findAt(ECS::Registry& world, int x, int y) {
            for (auto [entity, tag, position] : world.view<T, Position>()) {
                (void)tag;
                if (position.x == x && position.y == y)
                    return entity;
            }
            return ECS::Entity{};
        }

        template<typename T>
        bool isAt(ECS::Registry& world, int x, int y) {
            return world.isAlive(findAt<T>(world, x, y));
        }
    }

    /** @brief Reads the keys (arrows, WASD or ZQSD) and writes the player's MoveIntent. */
    class InputSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double) override {
                if (!hasService<Terminal>(world))
                    return;   // no terminal (editor's Play): no input
                Terminal& terminal = getService<Terminal>(world);
                int dx = 0;
                int dy = 0;
                for (int key = terminal.readKey(); key != ERR; key = terminal.readKey()) {
                    switch (key) {   // the last direction of the frame wins
                        case KEY_UP: case 'w': case 'z': dx = 0; dy = -1; break;
                        case KEY_DOWN: case 's': dx = 0; dy = 1; break;
                        case KEY_LEFT: case 'a': case 'q': dx = -1; dy = 0; break;
                        case KEY_RIGHT: case 'd': dx = 1; dy = 0; break;
                        default: break;
                    }
                }
                for (auto [entity, player, intent] : world.view<Player, MoveIntent>()) {
                    (void)entity;
                    (void)player;
                    intent = MoveIntent{dx, dy};
                }
            }
            const std::string name() const override { return "demo::InputSystem"; }
    };

    /** @brief Applies the MoveIntent: moves the player, pushes a box; a wall or a second box blocks. */
    class MoveSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double) override {
                for (auto [entity, player, position, intent] : world.view<Player, Position, MoveIntent>()) {
                    (void)entity;
                    (void)player;
                    const int dx = intent.dx;
                    const int dy = intent.dy;
                    intent = MoveIntent{};
                    if (dx == 0 && dy == 0)
                        continue;
                    const int x = position.x + dx;
                    const int y = position.y + dy;
                    if (detail::isAt<Wall>(world, x, y))
                        continue;
                    const ECS::Entity box = detail::findAt<Box>(world, x, y);
                    if (world.isAlive(box)) {
                        const int bx = x + dx;
                        const int by = y + dy;
                        if (detail::isAt<Wall>(world, bx, by) || detail::isAt<Box>(world, bx, by))
                            continue;   // the box cannot move: neither can the player
                        world.getComponent<Position>(box) = Position{bx, by};
                    }
                    position = Position{x, y};
                }
            }
            const std::string name() const override { return "demo::MoveSystem"; }
    };

    /** @brief Counts the boxes on a goal into the Progress resource. */
    class ProgressSystem : public ECS::ISystem {
        public:
            void init(ECS::Registry& world) override { world.setResource<Progress>(); }
            void update(ECS::Registry& world, double) override {
                Progress progress;
                for (auto [entity, box, position] : world.view<Box, Position>()) {
                    (void)entity;
                    (void)box;
                    ++progress.total;
                    if (detail::isAt<Goal>(world, position.x, position.y))
                        ++progress.placed;
                }
                world.getResource<Progress>() = progress;
            }
            const std::string name() const override { return "demo::ProgressSystem"; }
    };

    /** @brief Draws the level and the progress in the terminal. */
    class RenderSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double) override {
                if (!hasService<Terminal>(world))
                    return;   // no terminal: nothing to draw on
                getService<Terminal>(world).start();
                erase();
                // Drawn in this order, each one over the previous: goals, walls, boxes, player.
                draw<Goal>(world, '.');
                draw<Wall>(world, '#');
                for (auto [entity, box, position] : world.view<Box, Position>()) {
                    (void)entity;
                    (void)box;
                    put(position, detail::isAt<Goal>(world, position.x, position.y) ? '*' : '$');
                }
                for (auto [entity, player, position] : world.view<Player, Position>()) {
                    (void)entity;
                    (void)player;
                    put(position, detail::isAt<Goal>(world, position.x, position.y) ? '+' : '@');
                }
                int bottom = 0;
                for (auto [entity, position] : world.view<Position>()) {
                    (void)entity;
                    bottom = position.y > bottom ? position.y : bottom;
                }
                const Progress progress = world.hasResource<Progress>() ? world.getResource<Progress>() : Progress{};
                mvprintw(bottom + 3, Left, "Boxes on goals: %d / %d", progress.placed, progress.total);
                mvprintw(bottom + 4, Left, "%s",
                         progress.solved() ? "Solved! Press Ctrl+C to quit."
                                           : "Arrows (or WASD / ZQSD) to move, Ctrl+C to quit.");
                refresh();
            }
            const std::string name() const override { return "demo::RenderSystem"; }

        private:
            static constexpr int Top = 1;
            static constexpr int Left = 2;

            static void put(const Position& position, char glyph) {
                mvaddch(Top + position.y, Left + position.x, static_cast<chtype>(glyph));
            }

            template<typename T>
            static void draw(ECS::Registry& world, char glyph) {
                for (auto [entity, tag, position] : world.view<T, Position>()) {
                    (void)entity;
                    (void)tag;
                    put(position, glyph);
                }
            }
    };
}

E_SYSTEM(demo::InputSystem)
E_SYSTEM(demo::MoveSystem)
E_SYSTEM(demo::ProgressSystem)
E_SYSTEM(demo::RenderSystem)
