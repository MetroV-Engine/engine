/*
    Created on: 9/22/26
    Filename: SdlWindowBridge.cpp
    Description: Extracts native window handles from an SDL_Window for backends that need them.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/SdlWindowBridge/SdlWindowBridge.hpp"

#include <SDL3/SDL.h>

#include <iostream>

namespace gfx {

    NativeWindow nativeWindowFromSdl(SDL_Window* window)
    {
        NativeWindow native;

        if (window == nullptr) {
            std::cerr << "[SdlWindowBridge] ERROR: null window\n";
            return native;
        }

        const SDL_PropertiesID props = SDL_GetWindowProperties(window);
        if (props == 0) {
            std::cerr << "[SdlWindowBridge] ERROR: SDL_GetWindowProperties failed: " << SDL_GetError() << "\n";
            return native;
        }

#if defined(__linux__)
        const char* driver = SDL_GetCurrentVideoDriver();
        if (driver == nullptr) {
            std::cerr << "[SdlWindowBridge] ERROR: no current video driver\n";
            return native;
        }

        if (SDL_strcmp(driver, "x11") == 0) {
            void* display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
            const Uint64 handle = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);

            if (display == nullptr || handle == 0) {
                std::cerr << "[SdlWindowBridge] ERROR: missing X11 handles\n";
                return native;
            }

            native.kind = NativeWindowKind::X11;
            native.ndt = display;
            native.nwh = reinterpret_cast<void*>(static_cast<uintptr_t>(handle));
            return native;
        }

        if (SDL_strcmp(driver, "wayland") == 0) {
            void* display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
            void* surface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);

            if (display == nullptr || surface == nullptr) {
                std::cerr << "[SdlWindowBridge] ERROR: missing Wayland handles\n";
                return native;
            }

            native.kind = NativeWindowKind::Wayland;
            native.ndt = display;
            native.nwh = surface;
            return native;
        }

        std::cerr << "[SdlWindowBridge] ERROR: unsupported Linux video driver: " << driver << "\n";
        return native;
#elif defined(_WIN32)
        void* hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        if (hwnd == nullptr) {
            std::cerr << "[SdlWindowBridge] ERROR: missing Win32 handle\n";
            return native;
        }

        native.kind = NativeWindowKind::Win32;
        native.nwh = hwnd;
        return native;
#elif defined(__APPLE__)
        (void)props;    // Metal needs a view of its own, not a window property
        // bgfx's Metal backend draws into a CAMetalLayer; SDL makes one with plain C calls.
        SDL_MetalView view = SDL_Metal_CreateView(window);
        if (view == nullptr) {
            std::cerr << "[SdlWindowBridge] ERROR: SDL_Metal_CreateView failed: " << SDL_GetError() << "\n";
            return native;
        }

        native.kind = NativeWindowKind::Cocoa;
        native.nwh = SDL_Metal_GetLayer(view);
        native.metalView = view;
        return native;
#else
        std::cerr << "[SdlWindowBridge] ERROR: unsupported platform\n";
        return native;
#endif
    }

}
