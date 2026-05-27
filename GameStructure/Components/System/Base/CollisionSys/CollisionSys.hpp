#ifndef PROTO_COLLISIONSYS_HPP
#define PROTO_COLLISIONSYS_HPP
#include "Entity/GameEntities.hpp"
#include "System/ISystem.hpp"
namespace ECS::System {
    class CollisionSys : public ISystem {
        public:
            CollisionSys() = default;
            void update(registry &r, float deltaTime, SDL_Window *win) override;
            [[nodiscard]] const char* getName() const override {return "CollisionSys";};
        private:
            bool checkAABBCollision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
            void handleCollision(Entity::Position &pos1, Entity::Collider &col1, Entity::Bounds &bounds1, size_t entity1,
                               Entity::Position &pos2, Entity::Collider &col2, Entity::Bounds &bounds2, size_t entity2);
    };
}
#endif
