/*
    Created on: 11/9/26
    Filename: Font.cpp
    Description: Glyph atlas baked from TTF bytes; lays out UTF-8 text as textured triangles.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/Font/Font.hpp"

#include "Renderer/IRenderer.hpp"
#include "Renderer/QuadBuilder/QuadBuilder.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
// Without stb_rect_pack first, stb_truetype silently falls back to its naive packer.
#include <stb_rect_pack.h>
#include <stb_truetype.h>

namespace gfx {

    namespace {

        struct GlyphRange {
            int first = 0;
            int count = 0;
        };

        constexpr std::array<GlyphRange, 4> GlyphRanges{ {
            { 32, 95 },
            { 160, 96 },
            { 0x152, 2 },
            { 0x20AC, 1 },
        } };

        constexpr size_t GlyphCount = [] {
            size_t total = 0;
            for (const GlyphRange& range : GlyphRanges) {
                total += static_cast<size_t>(range.count);
            }
            return total;
        }();

        // stb reads later ranges at the first missing glyph's index: the slack keeps that read in bounds.
        using PackedGlyphs = std::array<stbtt_packedchar, GlyphCount * 2>;

        constexpr std::array<int, 4> AtlasSizes{ 512, 1024, 2048, 4096 };
        // Must stay above 0: without padding, stb fails the whole pack on empty glyphs like the space.
        constexpr int AtlasPadding = 1;
        constexpr size_t MinFontBytes = 12;
        constexpr uint32_t FallbackCodepoint = '?';
        constexpr uint32_t MaxCodepoint = 0x10FFFF;

        bool packAtlas(const unsigned char* const ttf, const float pixelHeight, const int size,
                       std::vector<unsigned char>& coverage, PackedGlyphs& packed)
        {
            coverage.assign(static_cast<size_t>(size) * static_cast<size_t>(size), 0);

            stbtt_pack_context context{};
            if (stbtt_PackBegin(&context, coverage.data(), size, size, 0, AtlasPadding, nullptr) == 0) {
                return false;
            }

            std::array<stbtt_pack_range, GlyphRanges.size()> ranges{};
            size_t offset = 0;
            for (size_t i = 0; i < GlyphRanges.size(); ++i) {
                ranges[i].font_size = pixelHeight;
                ranges[i].first_unicode_codepoint_in_range = GlyphRanges[i].first;
                ranges[i].num_chars = GlyphRanges[i].count;
                ranges[i].chardata_for_range = packed.data() + offset;
                offset += static_cast<size_t>(GlyphRanges[i].count);
            }

            const int result = stbtt_PackFontRanges(&context, ttf, 0, ranges.data(), static_cast<int>(ranges.size()));
            stbtt_PackEnd(&context);

            return result != 0;
        }

        // Byte length of the sequence at index, or 0 if it is invalid.
        size_t decodeSequence(const std::string_view text, const size_t index, uint32_t& codepoint)
        {
            const auto lead = static_cast<unsigned char>(text[index]);

            size_t length = 0;
            uint32_t value = 0;
            uint32_t minimum = 0;
            if (lead < 0x80) {
                length = 1;
                value = lead;
            } else if ((lead & 0xE0) == 0xC0) {
                length = 2;
                value = lead & 0x1Fu;
                minimum = 0x80;
            } else if ((lead & 0xF0) == 0xE0) {
                length = 3;
                value = lead & 0x0Fu;
                minimum = 0x800;
            } else if ((lead & 0xF8) == 0xF0) {
                length = 4;
                value = lead & 0x07u;
                minimum = 0x10000;
            } else {
                return 0;
            }

            if (text.size() - index < length) {
                return 0;
            }

            for (size_t i = 1; i < length; ++i) {
                const auto next = static_cast<unsigned char>(text[index + i]);
                if ((next & 0xC0) != 0x80) {
                    return 0;
                }
                value = (value << 6) | (next & 0x3Fu);
            }

            const bool surrogate = value >= 0xD800 && value <= 0xDFFF;
            if (value < minimum || value > MaxCodepoint || surrogate) {
                return 0;
            }

            codepoint = value;
            return length;
        }

        // Moves index past one sequence, or one byte if invalid; false means there is no code point.
        bool nextCodepoint(const std::string_view text, size_t& index, uint32_t& codepoint)
        {
            const size_t length = decodeSequence(text, index, codepoint);
            index += std::max<size_t>(length, 1);
            return length != 0;
        }

        Vertex makeVertex(const float x, const float y, const float u, const float v, const Color color)
        {
            Vertex vertex;
            vertex.x = x;
            vertex.y = y;
            vertex.u = u;
            vertex.v = v;
            vertex.r = color.r;
            vertex.g = color.g;
            vertex.b = color.b;
            vertex.a = color.a;
            return vertex;
        }

    }

    std::unique_ptr<Font> Font::create(IRenderer& renderer, const std::span<const uint8_t> ttf, const float pixelHeight)
    {
        if (ttf.size() < MinFontBytes || !std::isfinite(pixelHeight) || pixelHeight <= 0.0f) {
            std::cerr << "[Font] ERROR: font data too short or pixel height not positive\n";
            return nullptr;
        }

        const int fontOffset = stbtt_GetFontOffsetForIndex(ttf.data(), 0);
        stbtt_fontinfo info{};
        if (fontOffset < 0 || stbtt_InitFont(&info, ttf.data(), fontOffset) == 0) {
            std::cerr << "[Font] ERROR: data is not a readable font\n";
            return nullptr;
        }

        std::vector<unsigned char> coverage;
        PackedGlyphs packed{};
        int atlasSize = 0;
        for (const int size : AtlasSizes) {
            if (packAtlas(ttf.data(), pixelHeight, size, coverage, packed)) {
                atlasSize = size;
                break;
            }
        }

        if (atlasSize == 0) {
            std::cerr << "[Font] ERROR: glyphs at pixel height " << pixelHeight << " do not fit a "
                      << AtlasSizes.back() << "x" << AtlasSizes.back() << " atlas\n";
            return nullptr;
        }

        std::vector<uint8_t> rgba(coverage.size() * 4, 255);
        for (size_t i = 0; i < coverage.size(); ++i) {
            rgba[i * 4 + 3] = coverage[i];
        }

        const TextureHandle atlasTexture = renderer.createTexture(rgba.data(), static_cast<uint32_t>(atlasSize),
                                                                  static_cast<uint32_t>(atlasSize));
        if (atlasTexture == TextureHandle::Invalid) {
            std::cerr << "[Font] ERROR: could not create the atlas texture\n";
            return nullptr;
        }

        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
        // The packer derives this same scale from the positive font_size above.
        const float scale = stbtt_ScaleForPixelHeight(&info, pixelHeight);

        const auto atlas = static_cast<float>(atlasSize);
        std::unordered_map<uint32_t, Glyph> glyphs;
        size_t next = 0;
        for (const GlyphRange& range : GlyphRanges) {
            for (int i = 0; i < range.count; ++i) {
                const stbtt_packedchar& source = packed[next++];
                const int codepoint = range.first + i;
                if (stbtt_FindGlyphIndex(&info, codepoint) == 0) {
                    continue;
                }

                Glyph glyph;
                glyph.x0 = source.xoff;
                glyph.y0 = source.yoff;
                glyph.x1 = source.xoff2;
                glyph.y1 = source.yoff2;
                glyph.u0 = static_cast<float>(source.x0) / atlas;
                glyph.v0 = static_cast<float>(source.y0) / atlas;
                glyph.u1 = static_cast<float>(source.x1) / atlas;
                glyph.v1 = static_cast<float>(source.y1) / atlas;
                glyph.advance = source.xadvance;
                glyph.hasBitmap = source.x1 > source.x0 && source.y1 > source.y0;
                glyphs.emplace(static_cast<uint32_t>(codepoint), glyph);
            }
        }

        const float scaledAscent = scale * static_cast<float>(ascent);
        const float lineSpacing = scale * static_cast<float>(ascent - descent + lineGap);

        return std::unique_ptr<Font>(new Font(renderer, atlasTexture, scaledAscent, lineSpacing, std::move(glyphs)));
    }

    Font::Font(IRenderer& renderer, const TextureHandle atlas, const float ascent, const float lineSpacing,
               std::unordered_map<uint32_t, Glyph> glyphs)
        : m_renderer(renderer)
        , m_texture(atlas)
        , m_ascent(ascent)
        , m_lineHeight(lineSpacing)
        , m_glyphs(std::move(glyphs))
    {
    }

    Font::~Font()
    {
        m_renderer.destroyTexture(m_texture);
    }

    const Font::Glyph* Font::findGlyph(const uint32_t codepoint) const
    {
        auto found = m_glyphs.find(codepoint);
        if (found == m_glyphs.end()) {
            found = m_glyphs.find(FallbackCodepoint);
        }

        return found != m_glyphs.end() ? &found->second : nullptr;
    }

    Vec2 Font::measure(const std::string_view text) const
    {
        if (text.empty()) {
            return Vec2{};
        }

        float width = 0.0f;
        float lineWidth = 0.0f;
        int lines = 1;

        size_t index = 0;
        uint32_t codepoint = 0;
        while (index < text.size()) {
            if (!nextCodepoint(text, index, codepoint)) {
                continue;
            }

            if (codepoint == '\n') {
                width = std::max(width, lineWidth);
                lineWidth = 0.0f;
                ++lines;
                continue;
            }

            if (const Glyph* glyph = findGlyph(codepoint)) {
                lineWidth += glyph->advance;
            }
        }

        return Vec2{ std::max(width, lineWidth), static_cast<float>(lines) * m_lineHeight };
    }

    void Font::appendText(TriangleMesh& mesh, const TextDraw& draw) const
    {
        float penX = draw.position.x;
        float baseline = draw.position.y + m_ascent;
        // Backends sample linearly, so a 1:1 glyph stays sharp only on whole screen pixels.
        const bool snapToPixels = draw.space == Space::Screen;

        size_t index = 0;
        uint32_t codepoint = 0;
        while (index < draw.text.size()) {
            if (!nextCodepoint(draw.text, index, codepoint)) {
                continue;
            }

            if (codepoint == '\n') {
                penX = draw.position.x;
                baseline += m_lineHeight;
                continue;
            }

            const Glyph* glyph = findGlyph(codepoint);
            if (glyph == nullptr) {
                continue;
            }

            if (glyph->hasBitmap) {
                if (mesh.vertices.size() + 4 > MaxMeshVertices) {
                    return;
                }

                const auto base = static_cast<uint16_t>(mesh.vertices.size());
                const float originX = snapToPixels ? std::round(penX) : penX;
                const float originY = snapToPixels ? std::round(baseline) : baseline;
                const float left = originX + glyph->x0;
                const float top = originY + glyph->y0;
                const float right = originX + glyph->x1;
                const float bottom = originY + glyph->y1;

                mesh.vertices.push_back(makeVertex(left, top, glyph->u0, glyph->v0, draw.color));
                mesh.vertices.push_back(makeVertex(right, top, glyph->u1, glyph->v0, draw.color));
                mesh.vertices.push_back(makeVertex(right, bottom, glyph->u1, glyph->v1, draw.color));
                mesh.vertices.push_back(makeVertex(left, bottom, glyph->u0, glyph->v1, draw.color));
                for (const uint16_t quadIndex : QuadIndices) {
                    mesh.indices.push_back(static_cast<uint16_t>(base + quadIndex));
                }
            }

            penX += glyph->advance;
        }
    }

    TextureHandle Font::texture() const
    {
        return m_texture;
    }

    float Font::lineHeight() const
    {
        return m_lineHeight;
    }

}
