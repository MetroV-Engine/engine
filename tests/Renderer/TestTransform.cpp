#include <boost/ut.hpp>

#include "Support/Expect.hpp"
#include "Renderer/Transform/Transform.hpp"

#include <cmath>

using namespace boost::ut;

namespace {

    constexpr gfx::Vec2 Viewport{ 1280.0f, 720.0f };

    gfx::Camera2D makeCamera(const float x, const float y, const float zoom)
    {
        gfx::Camera2D camera;
        camera.center = gfx::Vec2{ x, y };
        camera.zoom = zoom;

        return camera;
    }

}

void run_transform_tests()
{
    "transform: camera center lands on viewport center"_test = [] {
        const gfx::Camera2D camera = makeCamera(500.0f, -300.0f, 2.5f);
        const gfx::Vec2 screen = gfx::worldToScreen(camera, camera.center, Viewport);

        expect(test::floatEq(screen.x, 640.0f));
        expect(test::floatEq(screen.y, 360.0f));
    };

    "transform: camera on half viewport is identity"_test = [] {
        const gfx::Camera2D camera = makeCamera(640.0f, 360.0f, 1.0f);
        const gfx::Vec2 screen = gfx::worldToScreen(camera, gfx::Vec2{ 100.0f, 50.0f }, Viewport);

        expect(test::floatEq(screen.x, 100.0f));
        expect(test::floatEq(screen.y, 50.0f));
    };

    "transform: zoom scales distance from center"_test = [] {
        const gfx::Camera2D camera = makeCamera(0.0f, 0.0f, 2.0f);
        const gfx::Vec2 screen = gfx::worldToScreen(camera, gfx::Vec2{ 10.0f, -5.0f }, Viewport);

        expect(test::floatEq(screen.x, 660.0f));
        expect(test::floatEq(screen.y, 350.0f));
    };

    "transform: rect moves top left and scales size"_test = [] {
        const gfx::Camera2D camera = makeCamera(0.0f, 0.0f, 2.0f);
        const gfx::Rect screen = gfx::worldToScreen(camera, gfx::Rect{ 10.0f, 20.0f, 30.0f, 40.0f }, Viewport);

        expect(test::floatEq(screen.x, 660.0f));
        expect(test::floatEq(screen.y, 400.0f));
        expect(test::floatEq(screen.w, 60.0f));
        expect(test::floatEq(screen.h, 80.0f));
    };

    "transform: screen to world inverts world to screen"_test = [] {
        const gfx::Camera2D cameras[] = {
            makeCamera(0.0f, 0.0f, 1.0f),
            makeCamera(-750.0f, 1200.0f, 0.2f),
            makeCamera(42.0f, -17.0f, 8.0f),
        };
        const gfx::Vec2 points[] = { { 0.0f, 0.0f }, { 123.5f, -987.25f }, { -4000.0f, 600.0f } };

        for (const gfx::Camera2D& camera : cameras) {
            for (const gfx::Vec2& world : points) {
                const gfx::Vec2 back = gfx::screenToWorld(camera, gfx::worldToScreen(camera, world, Viewport), Viewport);

                expect(test::near(back.x, world.x, 1e-2f));
                expect(test::near(back.y, world.y, 1e-2f));
            }
        }
    };

    "transform: non positive zoom acts as one"_test = [] {
        const gfx::Camera2D unit = makeCamera(10.0f, 20.0f, 1.0f);
        const gfx::Vec2 world{ 300.0f, -40.0f };
        const gfx::Vec2 expected = gfx::worldToScreen(unit, world, Viewport);

        for (const float zoom : { 0.0f, -2.0f }) {
            const gfx::Camera2D camera = makeCamera(10.0f, 20.0f, zoom);
            const gfx::Vec2 screen = gfx::worldToScreen(camera, world, Viewport);
            const gfx::Vec2 back = gfx::screenToWorld(camera, screen, Viewport);

            expect(test::floatEq(screen.x, expected.x));
            expect(test::floatEq(screen.y, expected.y));
            expect(std::isfinite(back.x) && std::isfinite(back.y));
            expect(test::floatEq(back.x, world.x));
            expect(test::floatEq(back.y, world.y));
        }
    };
}
