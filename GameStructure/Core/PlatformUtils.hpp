#ifndef PROTO_PLATFORM_UTILS_HPP
#define PROTO_PLATFORM_UTILS_HPP

#include <SDL3/SDL.h>

namespace Game {
    void *getNativeWindowHandle(SDL_Window *window);
    void *getNativeDisplayHandle(SDL_Window *window);
}

#endif
