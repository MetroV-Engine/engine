/*
    Created on: 11/17/26
    Filename: TextureCache.hpp
    Description: Sprite sheets by asset name, loaded on first use, owned until the cache dies.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "Renderer/IRenderer.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>

// A world resource is not used: the cache holds textures on a renderer and must die before it.
class TextureCache
{
public:
    // assetDirectory ends with a separator; names are appended to it.
    TextureCache(gfx::IRenderer &renderer, std::string assetDirectory);
    ~TextureCache();

    TextureCache(const TextureCache &) = delete;
    TextureCache &operator=(const TextureCache &) = delete;

    // Invalid for an empty name or a sheet that cannot be loaded (logged once per name).
    gfx::TextureHandle get(const std::string &name);

private:
    gfx::IRenderer &_renderer;
    std::string _directory;
    std::unordered_map<std::string, gfx::TextureHandle> _textures;
    std::unordered_set<std::string> _failed;
};
