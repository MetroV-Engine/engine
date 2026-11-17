/*
    Created on: 11/17/26
    Filename: TextureCache.cpp
    Description: Sprite sheets by asset name, loaded on first use, owned until the cache dies.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "TextureCache.hpp"

#include "Renderer/ImageLoader/ImageLoader.hpp"

#include <iostream>
#include <utility>

TextureCache::TextureCache(gfx::IRenderer &renderer, std::string assetDirectory)
    : _renderer(renderer), _directory(std::move(assetDirectory))
{
}

TextureCache::~TextureCache()
{
    for (const auto &[name, texture] : _textures)
        _renderer.destroyTexture(texture);
}

gfx::TextureHandle TextureCache::get(const std::string &name)
{
    if (name.empty() || _failed.contains(name))
        return gfx::TextureHandle::Invalid;
    if (const auto found = _textures.find(name); found != _textures.end())
        return found->second;

    const std::optional<tools::Image> image = tools::loadPng(_directory + name);
    const gfx::TextureHandle texture = image.has_value()
        ? _renderer.createTexture(image->rgba.data(), image->width, image->height)
        : gfx::TextureHandle::Invalid;
    if (texture == gfx::TextureHandle::Invalid) {
        std::cerr << "[TextureCache] WARN: no texture for \"" << name << "\": it draws nothing\n";
        _failed.insert(name);
        return gfx::TextureHandle::Invalid;
    }
    _textures.emplace(name, texture);
    return texture;
}
