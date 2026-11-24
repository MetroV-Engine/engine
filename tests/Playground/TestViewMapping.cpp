#include <boost/ut.hpp>

#include "Rendering/ViewMapping/ViewMapping.hpp"

#include "Renderer/Transform/Transform.hpp"

#include <cmath>

using namespace boost::ut;

namespace {

bool close(float a, float b)
{
    return std::abs(a - b) < 1e-3f;
}

} // namespace

void run_view_mapping_tests()
{
    "view mapping: a world point lands where the renderer draws it in the panel"_test = [] {
        const gfx::Camera2D camera{{120.0f, 360.0f}, 2.0f};
        const gfx::Vec2 topLeft{30.0f, 50.0f};
        const gfx::Vec2 size{800.0f, 600.0f};
        const ViewMapping mapping = viewMapping(camera, topLeft, size);
        for (const gfx::Vec2 world : {gfx::Vec2{0.0f, 0.0f}, gfx::Vec2{120.0f, 360.0f}, gfx::Vec2{-45.5f, 900.0f}}) {
            const gfx::Vec2 drawn = gfx::worldToScreen(camera, world, size);
            expect(close(mapping.origin.x + world.x * mapping.scale, topLeft.x + drawn.x));
            expect(close(mapping.origin.y + world.y * mapping.scale, topLeft.y + drawn.y));
        }
    };

    "view mapping: the camera centre is the middle of the panel"_test = [] {
        const ViewMapping mapping = viewMapping({{100.0f, 200.0f}, 1.0f}, {0.0f, 0.0f}, {640.0f, 480.0f});
        expect(close(mapping.origin.x + 100.0f * mapping.scale, 320.0f));
        expect(close(mapping.origin.y + 200.0f * mapping.scale, 240.0f));
    };

    "view mapping: a zoom of zero or less acts as one, as the renderer does"_test = [] {
        expect(close(viewMapping({{0.0f, 0.0f}, 0.0f}, {0.0f, 0.0f}, {10.0f, 10.0f}).scale, 1.0f));
        expect(close(viewMapping({{0.0f, 0.0f}, -2.0f}, {0.0f, 0.0f}, {10.0f, 10.0f}).scale, 1.0f));
    };
}
