#include <boost/ut.hpp>

#include "Renderer/ImageLoader/ImageLoader.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

using namespace boost::ut;

void run_image_loader_tests()
{
    "image loader: a sprite sheet loads as packed RGBA8"_test = [] {
        const auto image = tools::loadPng(RENDER_TEST_ASSETS_DIR "/bongo_repos_spritesheet.png");
        expect(image.has_value()) << fatal;
        expect(image->width == 6144_u && image->height == 512_u);
        expect(image->rgba.size() == std::size_t{image->width} * image->height * 4);
    };

    "image loader: pixels come back in RGBA byte order, whatever format the PNG stored"_test = [] {
        // A 2x1 ARGB8888 surface saved as PNG: opaque red, then half-transparent green.
        SDL_Surface *surface = SDL_CreateSurface(2, 1, SDL_PIXELFORMAT_ARGB8888);
        expect(surface != nullptr) << SDL_GetError() << fatal;
        SDL_WriteSurfacePixel(surface, 0, 0, 255, 0, 0, 255);
        SDL_WriteSurfacePixel(surface, 1, 0, 0, 255, 0, 128);
        const std::string path = (std::filesystem::temp_directory_path() / "tests_render_rgba_order.png").string();
        const bool saved = SDL_SavePNG(surface, path.c_str());
        SDL_DestroySurface(surface);
        expect(saved) << SDL_GetError() << fatal;

        const auto image = tools::loadPng(path);
        std::filesystem::remove(path);
        expect(image.has_value()) << fatal;
        expect(image->width == 2_u && image->height == 1_u);
        const std::array<uint8_t, 8> expected{255, 0, 0, 255, 0, 255, 0, 128};
        expect(image->rgba.size() == expected.size()) << fatal;
        for (std::size_t i = 0; i < expected.size(); ++i)
            expect(image->rgba[i] == expected[i]) << "byte " << i;
    };

    "image loader: a missing file fails cleanly"_test = [] {
        expect(!tools::loadPng(RENDER_TEST_ASSETS_DIR "/does-not-exist.png").has_value());
    };

    "image loader: a file that is not a PNG fails cleanly"_test = [] {
        expect(!tools::loadPng(RENDER_TEST_ASSETS_DIR "/DejaVuSans.ttf").has_value());
    };
}
