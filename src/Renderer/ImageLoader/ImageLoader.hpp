/*
    Created on: 10/4/26
    Filename: ImageLoader.hpp
    Description: Decodes PNG files into tightly packed RGBA8 for IRenderer::createTexture.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_IMAGELOADER_HPP
#define RENDERER_WRAPPER_IMAGELOADER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tools {

    // Tightly packed RGBA8, top row first: exactly what IRenderer::createTexture takes.
    struct Image {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<uint8_t> rgba;
    };

    // Returns nullopt on failure, after logging the reason.
    std::optional<Image> loadPng(const std::string& path);

}

#endif //RENDERER_WRAPPER_IMAGELOADER_HPP
