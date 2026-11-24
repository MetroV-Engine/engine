# Renderer

The engine's 2D renderer: a backend-agnostic interface, `gfx::IRenderer`, and its backends. Engine and
editor code draws through the interface, so the backend can be swapped without touching them. The
backend is picked once, at startup, with `gfx::makeRenderer` (`RendererFactory`).

It comes from [MetroV-Engine/renderer-wrapper](https://github.com/MetroV-Engine/renderer-wrapper)
(commit `7235a70`), where its history stays.

## Backends

| Backend | State |
| --- | --- |
| `Backend::Sdl3Cpu` (`SdlRenderer`) | Built. Draws through an `SDL_Renderer` it creates on the window, or offscreen (see below). |
| `Backend::Null` (`NullRenderer`) | Built. Records the draw calls without a window: for tests. |
| `Backend::Bgfx` (`BgfxRenderer`) | Built with `ENGINE_RENDERER_BGFX` (see Build). Draws through bgfx on the window, or offscreen. Without the switch, `makeRenderer` logs an error and returns `nullptr`. |

## Running it

From the engine's root, the playground shows the renderer in the editor:

```sh
python project_scripts/build.py --preset all      # SDL3 only (the default build)
./build/all/playground

cmake --preset bgfx && cmake --build --preset bgfx    # with bgfx (slow first build, see Build)
./build/bgfx/playground --backend=bgfx
```

`editor` and `gui_ecs_example` take the same `--backend=sdl|bgfx` option. In bgfx mode, bgfx draws both the
editor UI and the game; SDL3 keeps the window, events and input. Without the option, everything is drawn by
SDL3.

## Usage

Link the target and include headers from `src/`:

```cmake
target_link_libraries(my_game PRIVATE engine_render)
```

A complete program: a window, a texture, an animated sprite, shapes and text. It builds and runs as is, with
the playground's assets in `assets/`.

```cpp
#include "Renderer/Colors/Colors.hpp"
#include "Renderer/Font/Font.hpp"
#include "Renderer/ImageLoader/ImageLoader.hpp"
#include "Renderer/RendererFactory/RendererFactory.hpp"
#include "Renderer/SpriteAnimation/SpriteAnimation.hpp"

#include <SDL3/SDL.h>

#include <fstream>
#include <iterator>
#include <vector>

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Renderer demo", 1280, 720, SDL_WINDOW_RESIZABLE);

    // Backend::Sdl3Cpu, or Backend::Bgfx in a build with ENGINE_RENDERER_BGFX=ON.
    auto renderer = gfx::makeRenderer(gfx::Backend::Sdl3Cpu, window, 1280, 720);
    if (renderer == nullptr)
        return 1; // the reason is already logged

    // A texture: tightly packed RGBA8 pixels, copied by the renderer.
    const auto sheet = tools::loadPng("assets/bongo_repos_spritesheet.png");
    const gfx::TextureHandle bongo =
        sheet ? renderer->createTexture(sheet->rgba.data(), sheet->width, sheet->height) : gfx::TextureHandle::Invalid;
    gfx::SpriteAnimation idle({512.0f, 512.0f}, 12, 10.0f); // 12 frames of 512x512, 10 per second

    // A font: the bytes of a .ttf file, baked into an atlas texture on this renderer.
    std::ifstream file("assets/DejaVuSans.ttf", std::ios::binary);
    const std::vector<uint8_t> ttf{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    auto font = gfx::Font::create(*renderer, ttf, 24.0f);

    gfx::Camera2D camera{{640.0f, 360.0f}, 1.0f}; // the world point shown at the middle of the window
    uint64_t last = SDL_GetTicks();
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
                renderer->resize(static_cast<uint32_t>(event.window.data1), static_cast<uint32_t>(event.window.data2));
        }
        const uint64_t now = SDL_GetTicks();
        idle.update(static_cast<float>(now - last) / 1000.0f);
        last = now;

        renderer->beginFrame(gfx::fromHex(0x181A20FF));
        renderer->setCamera(camera);

        // World space (the default): moves and zooms with the camera.
        renderer->drawRect({{100.0f, 500.0f, 1080.0f, 40.0f}, gfx::fromHex(0x3A7D44FF)});
        renderer->drawCircle({{900.0f, 300.0f}, 50.0f, gfx::Blue});
        renderer->drawLine({{100.0f, 100.0f}, {400.0f, 250.0f}, 4.0f, gfx::Red});

        gfx::SpriteDraw sprite;
        sprite.texture = bongo;
        sprite.source = idle.sourceRect();
        sprite.dest = {500.0f, 244.0f, 256.0f, 256.0f};
        renderer->drawSprite(sprite);

        // Screen space: pixels from the window's top-left, ignores the camera.
        gfx::TextDraw hud;
        hud.font = font.get();
        hud.text = "Hello from IRenderer";
        hud.position = {16.0f, 16.0f};
        hud.space = gfx::Space::Screen;
        hud.layer = gfx::Layer::Foreground;
        renderer->drawText(hud);

        renderer->endFrame(); // draws and presents
    }

    font.reset(); // a font holds its renderer: destroy it first
    renderer.reset();
    SDL_DestroyWindow(window);
    SDL_Quit();
}
```

### The frame

Every draw call goes between `beginFrame(clearColour)` and `endFrame()`. A draw outside a frame is ignored,
with one warning. The renderer queues the calls and draws them all in `endFrame()`, which also presents the
window. Call `resize(width, height)` when the window's pixel size changes. It does not move the camera.

### Coordinates, camera and layers

- `Space::World` (the default) goes through the camera: `Camera2D::center` is the world point shown at the
  middle of the viewport, and `zoom` above 1 magnifies. The camera set last before `endFrame()` applies to
  the whole frame. It starts centred on the viewport, so world and screen coordinates match until you move
  it.
- `Space::Screen` is in pixels from the top-left: use it for the HUD.
- Y points down, rotations are in radians, clockwise.
- `Layer::Background`, then `Game` (the default), then `Foreground`, whatever the call order. Within a layer,
  the calls draw in order.
- `gfx::worldToScreen` and `gfx::screenToWorld` (`Transform/Transform.hpp`) convert points, for example to
  pick objects with the mouse.

### Draw calls

| Call | Draws |
| --- | --- |
| `drawRect({dest, color})` | A filled rectangle. |
| `drawSprite(SpriteDraw)` | A texture into `dest`. `source` picks part of it, in pixels (empty means all of it). It also takes `rotation`, `flipX` and a `tint`. |
| `drawLine({from, to, thickness, color})` | A thick segment. |
| `drawTriangle({points, color})`, `drawQuad({corners, color})` | Filled shapes. A quad's corners go in order around it. |
| `drawCircle({center, radius, color, segments})` | A filled circle. `segments <= 0` picks the count from the radius. |
| `drawPolygon({points, color})` | A filled polygon, convex or not. The points are only read during the call. It draws nothing under 3 points, or when the points cannot be triangulated. |
| `drawMesh({vertices, indices, texture})` | Your own triangles, coloured per vertex, textured or not. |
| `drawText({font, text, position, color})` | UTF-8 text with a `gfx::Font`. `position` is the top-left of the first line, and `\n` starts a new line. |

Every call also takes `layer` and `space`. The structs live in `RendererTypes.hpp`, and the colour helpers
(`fromHex`, named colours, HSV, `lerp`) in `Colors/Colors.hpp`.

### Resources

- **Textures:** `createTexture(rgba, width, height)` copies the pixels and returns a `TextureHandle`.
  `TextureHandle::Invalid` means it failed. `destroyTexture` frees one. A texture destroyed during a frame
  still draws in that frame. `tools::loadPng(path)` gives the RGBA8 pixels of a PNG file.
- **Fonts:** `gfx::Font::create(renderer, ttfBytes, pixelHeight)` bakes a font into a texture on that
  renderer. It returns `nullptr` for bad input. A font must be destroyed before its renderer. `measure(text)`
  gives the size of a text, for example to centre it.
- **Animation:** `gfx::SpriteAnimation(frameSize, frameCount, fps)` steps through frames laid out left to
  right on a sheet. Call `update(dt)` every frame and use `sourceRect()` as a sprite's `source`.
- **Asset paths:** `tools::pathNextToExecutable("assets/x.png")` resolves a path next to the binary, which is
  where the playground's build copies its assets.

### Tests

`gfx::makeRenderer(gfx::Backend::Null, nullptr, w, h)` returns a `NullRenderer`. It needs no window or GPU, and
it records every call, so a test can check what a system drew.

## Offscreen rendering

`gfx::makeOffscreenRenderer(backend, sdlRenderer, width, height)` returns an `IOffscreenRenderer`. It draws
into a texture on a renderer that someone else owns, such as the editor's, instead of presenting to a
window. `outputTexture()` gives that texture as an opaque handle (an `SDL_Texture*` for SDL), ready for
`ImGui::Image`. Each frame puts the borrowed renderer's target, blend mode and texture address mode back
as they were.

`gfx::makeOffscreenRenderer(bgfxContext, width, height)` is the bgfx equivalent. It draws into a bgfx
framebuffer of that size, recreated on `resize`, on a view of its own. `outputTexture()` gives the
framebuffer's colour texture in the id encoding of `BgfxContext/BgfxTextureId.hpp` (`idx + 1`, so 0 means
none), the one the editor's bgfx ImGui backend decodes.

### One bgfx per process

bgfx is global, so one `BgfxContext` owns it: `bgfx::init` on an SDL window, `reset` on resize, `frame`,
`shutdown`, and the view ids (game views from 0 up, `BgfxContext::UiView` = 255 for the UI). Only one can
exist at a time. In window mode (`makeRenderer(Backend::Bgfx, ...)`) the renderer owns its context and calls
`frame()` itself. An offscreen renderer borrows the context of whoever owns it, such as the editor's UI,
and never calls `frame()`. Under SDL's `dummy` video driver, or with no window, the context starts bgfx's
Noop device: it draws nothing, which is how the tests run bgfx without a GPU.

## Build

`engine_render` is built when `ENGINE_BUILD_RENDERER` is ON (the default). vcpkg then installs the
`renderer` feature of the manifest (SDL3 and stb). Like the ECS, it is included from `src/`:
`#include "Renderer/IRenderer.hpp"`. It does not depend on the ECS.

`ENGINE_RENDERER_BGFX` (OFF by default) adds the bgfx backend. It fetches bgfx.cmake (pinned in
`cmake/Bgfx.cmake`) and builds bgfx's shader compiler, which takes a long time on the first build. The
sprite shaders in `shaders/` are compiled for each platform's default renderer: GLSL 430 and SPIR-V on
Linux, Direct3D 11 and SPIR-V on Windows, Metal on macOS. Use the `bgfx` and `bgfx-tests` presets. With the
switch OFF nothing about the build changes.

`./build/bgfx/playground --backend=bgfx` runs the playground with both the editor UI and the game drawn by
bgfx (see Running it). The manual workflow `.github/workflows/bgfx-compile.yml` compiles and tests this build on Linux,
Windows and macOS: start it from the Actions tab, or push to the `bgfx-check` branch.

Its tests are in `tests/Renderer` (`tests_render`). They run headless (SDL's software renderer and its `dummy`
video driver) and count towards the 90 % line coverage CI requires over `src/`. The bgfx files and
`SdlWindowBridge` are only built with `ENGINE_RENDERER_BGFX=ON`, so the `coverage` preset does not measure them.

## Layout

Each unit with a `.cpp` lives in a folder of its own name with its header
(`Font/Font.hpp`, `Font/Font.cpp`). Header-only files (`IRenderer.hpp`, `RendererTypes.hpp`) stay at the
root of the module. New `.cpp` files are picked up by CMake automatically. Public headers include no SDL
or bgfx header: `IRenderer.hpp` is backend-free, and `RendererFactory.hpp` / `SdlRenderer.hpp` only
forward-declare the SDL and bgfx types they take (`SDL_Window`, `BgfxContext`...).
