/*
    Created on: 9/22/26
    Filename: NullRenderer.cpp
    Description: Headless backend: records draw calls so systems can be tested without a window.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/NullRenderer/NullRenderer.hpp"

#include "Renderer/ShapeBuilder/ShapeBuilder.hpp"

#include <utility>

namespace gfx {

    NullRenderer::NullRenderer(const uint32_t width, const uint32_t height)
        : m_width(width)
        , m_height(height)
    {
        m_camera.center = Vec2{ static_cast<float>(width) * 0.5f, static_cast<float>(height) * 0.5f };
    }

    void NullRenderer::resize(const uint32_t width, const uint32_t height)
    {
        if (width == 0 || height == 0) {
            return;
        }

        m_width = width;
        m_height = height;
    }

    void NullRenderer::beginFrame(const Color clear)
    {
        m_lastClear = clear;
        m_inFrame = true;
    }

    void NullRenderer::endFrame()
    {
        m_inFrame = false;
        ++m_framesPresented;
    }

    void NullRenderer::setCamera(const Camera2D& camera)
    {
        m_camera = camera;
    }

    TextureHandle NullRenderer::createTexture(const uint8_t* rgba, const uint32_t width, const uint32_t height)
    {
        if (rgba == nullptr || width == 0 || height == 0) {
            return TextureHandle::Invalid;
        }

        const uint32_t id = m_nextTexture++;
        m_textures.emplace(id, Vec2{ static_cast<float>(width), static_cast<float>(height) });

        return static_cast<TextureHandle>(id);
    }

    void NullRenderer::destroyTexture(const TextureHandle texture)
    {
        m_textures.erase(static_cast<uint32_t>(texture));
    }

    Vec2 NullRenderer::textureSize(const TextureHandle texture) const
    {
        const auto found = m_textures.find(static_cast<uint32_t>(texture));

        return found != m_textures.end() ? found->second : Vec2{ 0.0f, 0.0f };
    }

    void NullRenderer::drawRect(const RectDraw& draw)
    {
        if (!m_inFrame) {
            return;
        }

        Recorded entry;
        entry.kind = Recorded::Kind::Rect;
        entry.rect = draw;
        m_recorded.push_back(entry);
    }

    void NullRenderer::drawSprite(const SpriteDraw& draw)
    {
        if (!m_inFrame) {
            return;
        }

        Recorded entry;
        entry.kind = Recorded::Kind::Sprite;
        entry.sprite = draw;
        m_recorded.push_back(entry);
    }

    void NullRenderer::drawLine(const LineDraw& draw)
    {
        if (!m_inFrame) {
            return;
        }

        Recorded entry;
        entry.kind = Recorded::Kind::Line;
        entry.line = draw;
        m_recorded.push_back(entry);
    }

    void NullRenderer::drawTriangle(const TriangleDraw& draw)
    {
        if (!m_inFrame) {
            return;
        }

        Recorded entry;
        entry.kind = Recorded::Kind::Triangle;
        entry.triangle = draw;
        m_recorded.push_back(entry);
    }

    void NullRenderer::drawQuad(const QuadDraw& draw)
    {
        if (!m_inFrame) {
            return;
        }

        Recorded entry;
        entry.kind = Recorded::Kind::Quad;
        entry.quad = draw;
        m_recorded.push_back(entry);
    }

    void NullRenderer::drawCircle(const CircleDraw& draw)
    {
        if (!m_inFrame) {
            return;
        }

        Recorded entry;
        entry.kind = Recorded::Kind::Circle;
        entry.circle = draw;
        m_recorded.push_back(entry);
    }

    void NullRenderer::drawPolygon(const PolygonDraw& draw)
    {
        if (!m_inFrame) {
            return;
        }

        Recorded entry;
        entry.kind = Recorded::Kind::Polygon;
        entry.polygon = draw;
        entry.polygon.points = {};
        entry.polygonPoints.assign(draw.points.begin(), draw.points.end());
        m_recorded.push_back(std::move(entry));
    }

    void NullRenderer::drawMesh(const MeshDraw& draw)
    {
        if (draw.vertices.empty() || draw.indices.empty() || !m_inFrame) {
            return;
        }

        const bool knownTexture = draw.texture == TextureHandle::Invalid
            || m_textures.contains(static_cast<uint32_t>(draw.texture));
        if (!isValidMesh(draw.vertices, draw.indices) || !knownTexture) {
            return;
        }

        Recorded entry;
        entry.kind = Recorded::Kind::Mesh;
        entry.meshVertices.assign(draw.vertices.begin(), draw.vertices.end());
        entry.meshIndices.assign(draw.indices.begin(), draw.indices.end());
        entry.meshTexture = draw.texture;
        entry.meshLayer = draw.layer;
        entry.meshSpace = draw.space;
        m_recorded.push_back(std::move(entry));
    }

    void NullRenderer::drawText(const TextDraw& draw)
    {
        if (!m_inFrame) {
            return;
        }

        Recorded entry;
        entry.kind = Recorded::Kind::Text;
        entry.text = draw;
        entry.text.text = {};
        entry.textContent.assign(draw.text);
        m_recorded.push_back(std::move(entry));
    }

    const char* NullRenderer::backendName() const
    {
        return "null";
    }

}
