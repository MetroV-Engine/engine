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
| `Backend::Bgfx` | Not built yet: `makeRenderer` logs an error and returns `nullptr`. |

## Offscreen rendering

`gfx::makeOffscreenRenderer(backend, sdlRenderer, width, height)` returns an `IOffscreenRenderer`. It draws
into a texture on a renderer that someone else owns, such as the editor's, instead of presenting to a
window. `outputTexture()` gives that texture as an opaque handle (an `SDL_Texture*` for SDL), ready for
`ImGui::Image`. Each frame puts the borrowed renderer's target, blend mode and texture address mode back
as they were. Only `Backend::Sdl3Cpu` is supported.

## Build

`engine_render` is built when `ENGINE_BUILD_RENDERER` is ON (the default). vcpkg then installs the
`renderer` feature of the manifest (SDL3 and stb). Like the ECS, it is included from `src/`:
`#include "Renderer/IRenderer.hpp"`. It does not depend on the ECS.

Its tests are in `tests/Renderer` (`tests_render`). The `coverage` preset turns the renderer OFF, because CI
requires 90 % line coverage over `src/` and most of the renderer has no tests yet.

## Layout

Each unit with a `.cpp` lives in a folder of its own name with its header
(`Font/Font.hpp`, `Font/Font.cpp`). Header-only files (`IRenderer.hpp`, `RendererTypes.hpp`) stay at the
root of the module. New `.cpp` files are picked up by CMake automatically. Public headers include no SDL
or bgfx header: `IRenderer.hpp` is backend-free, and `RendererFactory.hpp` / `SdlRenderer.hpp` only
forward-declare the SDL types they take (`SDL_Window`...).
