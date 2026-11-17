# Renderer

The engine's 2D renderer: a backend-agnostic interface, `gfx::IRenderer`, and its backends. Engine and
editor code draws through the interface, so the backend can be swapped without touching them. The
backend is picked once, at startup, with `gfx::makeRenderer` (`RendererFactory`).

It comes from [MetroV-Engine/renderer-wrapper](https://github.com/MetroV-Engine/renderer-wrapper)
(commit `7235a70`), where its history stays.

## Backends

| Backend | State |
| --- | --- |
| `Backend::Sdl3Cpu` (`SdlRenderer`) | Built. Draws through an `SDL_Renderer` it creates on the window. |
| `Backend::Null` (`NullRenderer`) | Built. Records the draw calls without a window: for tests. |
| `Backend::Bgfx` | Not built yet: `makeRenderer` logs an error and returns `nullptr`. |

## Build

`engine_render` is built when `ENGINE_BUILD_RENDERER` is ON (the default). vcpkg then installs the
`renderer` feature of the manifest (SDL3 and stb). Like the ECS, it is included from `src/`:
`#include "Renderer/IRenderer.hpp"`. It does not depend on the ECS.

The `coverage` preset turns it OFF until its tests exist: CI requires 90 % line coverage over `src/`.

## Layout

Each unit with a `.cpp` lives in a folder of its own name with its header
(`Font/Font.hpp`, `Font/Font.cpp`). Header-only files (`IRenderer.hpp`, `RendererTypes.hpp`) stay at the
root of the module. New `.cpp` files are picked up by CMake automatically. Public headers include no SDL
or bgfx header: `IRenderer.hpp` is backend-free, and `RendererFactory.hpp` / `SdlRenderer.hpp` only
forward-declare the SDL types they take (`SDL_Window`...).
