/*
    Created on: 10/4/26
    Filename: ImageLoader.cpp
    Description: Decodes PNG files into tightly packed RGBA8 for IRenderer::createTexture.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/ImageLoader/ImageLoader.hpp"

#include <SDL3/SDL.h>

#include <cstring>
#include <iostream>

namespace tools {

    std::optional<Image> loadPng(const std::string& path)
    {
        SDL_Surface* loaded = SDL_LoadPNG(path.c_str());
        if (loaded == nullptr) {
            std::cerr << "[ImageLoader] ERROR: cannot load " << path << ": " << SDL_GetError() << "\n";
            return std::nullopt;
        }

        // A PNG decodes to whatever format the file stored; the renderer wants RGBA byte order.
        SDL_Surface* converted = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(loaded);
        if (converted == nullptr) {
            std::cerr << "[ImageLoader] ERROR: cannot convert " << path << " to RGBA: " << SDL_GetError() << "\n";
            return std::nullopt;
        }

        if (!SDL_LockSurface(converted)) {
            std::cerr << "[ImageLoader] ERROR: cannot lock " << path << ": " << SDL_GetError() << "\n";
            SDL_DestroySurface(converted);
            return std::nullopt;
        }

        Image image;
        image.width = static_cast<uint32_t>(converted->w);
        image.height = static_cast<uint32_t>(converted->h);

        const size_t rowBytes = static_cast<size_t>(image.width) * 4;
        image.rgba.resize(rowBytes * image.height);

        // Surface rows may be padded (pitch > width * 4): copy row by row to pack them.
        const auto* source = static_cast<const uint8_t*>(converted->pixels);
        for (size_t row = 0; row < image.height; ++row) {
            std::memcpy(image.rgba.data() + row * rowBytes,
                source + row * static_cast<size_t>(converted->pitch), rowBytes);
        }

        SDL_UnlockSurface(converted);
        SDL_DestroySurface(converted);

        return image;
    }

}
