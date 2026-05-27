#include "PlatformUtils.hpp"
#include <SDL3/SDL_properties.h>

namespace Game {

void *getNativeWindowHandle(SDL_Window *window)
{
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    void *wl = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    if (wl) return wl;
    return (void *)(uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
}

void *getNativeDisplayHandle(SDL_Window *window)
{
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    void *wl = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
    if (wl) return wl;
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
}

}
