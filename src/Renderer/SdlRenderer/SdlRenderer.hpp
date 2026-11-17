/*
    Created on: 9/22/26
    Filename: SdlRenderer.hpp
    Description: SDL3 backend: the interim renderer the UI team builds against.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_SDLRENDERER_HPP
#define RENDERER_WRAPPER_SDLRENDERER_HPP

#include "Renderer/IRenderer.hpp"
#include "Renderer/ShapeBuilder/ShapeBuilder.hpp"

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Vertex;

namespace gfx {

    class SdlRenderer final : public IRenderer {
        public:
            // Returns nullptr on failure, after logging the reason.
            static std::unique_ptr<SdlRenderer> create(SDL_Window* window, uint32_t width, uint32_t height);

            ~SdlRenderer() override;

            void resize(uint32_t width, uint32_t height) override;

            void beginFrame(Color clear) override;
            void endFrame() override;

            void setCamera(const Camera2D& camera) override;

            TextureHandle createTexture(const uint8_t* rgba, uint32_t width, uint32_t height) override;
            void destroyTexture(TextureHandle texture) override;
            Vec2 textureSize(TextureHandle texture) const override;

            void drawRect(const RectDraw& draw) override;
            void drawSprite(const SpriteDraw& draw) override;
            void drawMesh(const MeshDraw& draw) override;
            void drawLine(const LineDraw& draw) override;
            void drawTriangle(const TriangleDraw& draw) override;
            void drawQuad(const QuadDraw& draw) override;
            void drawCircle(const CircleDraw& draw) override;
            void drawPolygon(const PolygonDraw& draw) override;
            void drawText(const TextDraw& draw) override;

            const char* backendName() const override;

        private:
            struct Command {
                enum class Kind : uint8_t { Rect, Sprite, Mesh };

                Kind kind = Kind::Rect;
                TextureHandle texture = TextureHandle::Invalid;
                Rect source;
                Rect dest;
                float rotation = 0.0f;
                bool flipX = false;
                Color color;
                Space space = Space::World;

                // Mesh only: ranges into the frame arenas.
                uint32_t firstVertex = 0;
                uint32_t vertexCount = 0;
                uint32_t firstIndex = 0;
                uint32_t indexCount = 0;
            };

            SdlRenderer(SDL_Renderer* renderer, uint32_t width, uint32_t height);

            bool inFrameOrWarn();
            void submit(const Command& command, Layer layer);
            void flush(const Command& command);
            void flushMesh(const Command& command);
            SDL_Texture* find(TextureHandle texture) const;
            void releasePending();
            void submitScratch(TextureHandle texture, Layer layer, Space space);

            SDL_Renderer* m_renderer = nullptr;
            std::string m_name;

            uint32_t m_width = 0;
            uint32_t m_height = 0;
            Camera2D m_camera;
            bool m_inFrame = false;

            std::array<std::vector<Command>, LayerCount> m_layers{};
            std::vector<Vertex> m_meshVertices;
            std::vector<uint16_t> m_meshIndices;
            // Reused across flushes; SDL_Vertex stays incomplete in this header, and every use is in the .cpp.
            std::vector<SDL_Vertex> m_geometryVertices;
            std::vector<int> m_geometryIndices;
            std::unordered_map<uint32_t, SDL_Texture*> m_textures;
            uint32_t m_nextTexture = 1;
            std::vector<TextureHandle> m_pendingDestroy;

            bool m_warnedOutsideFrame = false;
            bool m_warnedInvalidTexture = false;
            bool m_warnedInvalidMesh = false;
            bool m_warnedPolygon = false;
            bool m_warnedNullFont = false;

            TriangleMesh m_scratch;
    };

}

#endif //RENDERER_WRAPPER_SDLRENDERER_HPP
