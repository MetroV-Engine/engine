/*
    Created on: 11/9/26
    Filename: Font.hpp
    Description: Glyph atlas baked from TTF bytes; lays out UTF-8 text as textured triangles.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_FONT_HPP
#define RENDERER_WRAPPER_FONT_HPP

#include "Renderer/RendererTypes.hpp"
#include "Renderer/ShapeBuilder/ShapeBuilder.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <unordered_map>

namespace gfx {

    class IRenderer;

    // Holds its renderer by reference and must be destroyed before it.
    class Font {
        public:
            // ttf is only read during the call and must be a trusted font file (stb does not bounds-check); nullptr if the glyphs overflow a 4096 atlas.
            static std::unique_ptr<Font> create(IRenderer& renderer, std::span<const uint8_t> ttf, float pixelHeight);

            ~Font();

            Font(const Font&) = delete;
            Font& operator=(const Font&) = delete;

            Vec2 measure(std::string_view text) const;
            // Text that would pass MaxMeshVertices is cut off at that limit.
            void appendText(TriangleMesh& mesh, const TextDraw& draw) const;

            TextureHandle texture() const;
            float lineHeight() const;

        private:
            struct Glyph {
                float x0 = 0.0f;        // quad corners, relative to the pen on the baseline
                float y0 = 0.0f;
                float x1 = 0.0f;
                float y1 = 0.0f;
                float u0 = 0.0f;
                float v0 = 0.0f;
                float u1 = 0.0f;
                float v1 = 0.0f;
                float advance = 0.0f;
                bool hasBitmap = false;
            };

            Font(IRenderer& renderer, TextureHandle atlas, float ascent, float lineSpacing,
                 std::unordered_map<uint32_t, Glyph> glyphs);

            const Glyph* findGlyph(uint32_t codepoint) const;

            IRenderer& m_renderer;
            TextureHandle m_texture;
            float m_ascent;
            float m_lineHeight;
            std::unordered_map<uint32_t, Glyph> m_glyphs;
    };

}

#endif //RENDERER_WRAPPER_FONT_HPP
