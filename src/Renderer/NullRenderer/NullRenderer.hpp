/*
    Created on: 9/22/26
    Filename: NullRenderer.hpp
    Description: Headless backend: records draw calls so systems can be tested without a window.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_NULLRENDERER_HPP
#define RENDERER_WRAPPER_NULLRENDERER_HPP

#include "Renderer/IRenderer.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace gfx {

    class NullRenderer final : public IRenderer {
        public:
            struct Recorded {
                enum class Kind : uint8_t { Rect, Sprite, Line, Triangle, Quad, Circle, Polygon, Mesh, Text };

                Kind kind = Kind::Rect;
                RectDraw rect;
                SpriteDraw sprite;
                LineDraw line;
                TriangleDraw triangle;
                QuadDraw quad;
                CircleDraw circle;
                PolygonDraw polygon;        // points left empty; see polygonPoints
                TextDraw text;              // text left empty; see textContent
                std::vector<Vec2> polygonPoints;
                std::string textContent;
                std::vector<Vertex> meshVertices;
                std::vector<uint16_t> meshIndices;
                TextureHandle meshTexture = TextureHandle::Invalid;
                Layer meshLayer = Layer::Game;
                Space meshSpace = Space::World;
            };

            NullRenderer(uint32_t width, uint32_t height);

            void resize(uint32_t width, uint32_t height) override;

            void beginFrame(Color clear) override;
            void endFrame() override;

            void setCamera(const Camera2D& camera) override;

            TextureHandle createTexture(const uint8_t* rgba, uint32_t width, uint32_t height) override;
            void destroyTexture(TextureHandle texture) override;
            Vec2 textureSize(TextureHandle texture) const override;

            void drawRect(const RectDraw& draw) override;
            void drawSprite(const SpriteDraw& draw) override;
            void drawLine(const LineDraw& draw) override;
            void drawTriangle(const TriangleDraw& draw) override;
            void drawQuad(const QuadDraw& draw) override;
            void drawCircle(const CircleDraw& draw) override;
            void drawPolygon(const PolygonDraw& draw) override;
            void drawMesh(const MeshDraw& draw) override;
            void drawText(const TextDraw& draw) override;

            const char* backendName() const override;

            // Test surface.
            const std::vector<Recorded>& recorded() const { return m_recorded; }
            const Camera2D& camera() const { return m_camera; }
            Color lastClear() const { return m_lastClear; }
            uint32_t framesPresented() const { return m_framesPresented; }
            void clearRecorded() { m_recorded.clear(); }

        private:
            uint32_t m_width = 0;
            uint32_t m_height = 0;
            Camera2D m_camera;
            Color m_lastClear;
            bool m_inFrame = false;
            uint32_t m_framesPresented = 0;

            std::vector<Recorded> m_recorded;
            std::unordered_map<uint32_t, Vec2> m_textures;
            uint32_t m_nextTexture = 1;
    };

}

#endif //RENDERER_WRAPPER_NULLRENDERER_HPP
