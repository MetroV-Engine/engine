/*
    Created on: 10/4/26
    Filename: BgfxRenderer.hpp
    Description: bgfx backend: batched textured quads and meshes, camera as a GPU matrix.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_BGFXRENDERER_HPP
#define RENDERER_WRAPPER_BGFXRENDERER_HPP

#include "Renderer/IOffscreenRenderer.hpp"
#include "Renderer/BgfxContext/BgfxContext.hpp"
#include "Renderer/ShapeBuilder/ShapeBuilder.hpp"
#include "Renderer/BgfxProgram/BgfxProgram.hpp"
#include "Renderer/CameraMatrix/CameraMatrix.hpp"

#include <bgfx/bgfx.h>

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct SDL_Window;

namespace gfx {

    class BgfxRenderer final : public IOffscreenRenderer {
        public:
            // Window mode: owns the BgfxContext and presents. Returns nullptr on failure, after logging the reason.
            static std::unique_ptr<BgfxRenderer> create(SDL_Window* window, uint32_t width, uint32_t height);
            // Offscreen mode: draws into a framebuffer on a view of a context it borrows, and never calls
            // bgfx::frame() (the context's owner does). Returns nullptr on failure, after logging the reason.
            static std::unique_ptr<BgfxRenderer> createOffscreen(BgfxContext& context, uint32_t width, uint32_t height);

            ~BgfxRenderer() override;

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

            std::uintptr_t outputTexture() const override;

        private:
            struct Command {
                enum class Kind : uint8_t { Rect, Sprite, Mesh };

                Kind kind = Kind::Rect;
                RectDraw rect;
                SpriteDraw sprite;

                // Mesh only: ranges into the frame arenas.
                uint32_t firstVertex = 0;
                uint32_t vertexCount = 0;
                uint32_t firstIndex = 0;
                uint32_t indexCount = 0;
                TextureHandle texture = TextureHandle::Invalid;
                Space space = Space::World;
            };

            struct Texture {
                bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
                Vec2 size;
            };

            BgfxRenderer(std::unique_ptr<BgfxContext> owned, BgfxContext& context, uint32_t width, uint32_t height);

            static TextureHandle textureOf(const Command& command);
            static Space spaceOf(const Command& command);
            static uint32_t vertexCountOf(const Command& command);
            static uint32_t indexCountOf(const Command& command);

            bool createResources();
            bool createFramebuffer();
            bool inFrameOrWarn();
            void submit(const Command& command, Layer layer);
            void submitLayer(const std::vector<Command>& commands, const Mat4& camera, const Mat4& identity);
            void submitBatch(const Command* first, uint32_t count, const Mat4& camera, const Mat4& identity);
            const Texture* find(TextureHandle texture) const;
            void releasePending();
            void submitScratch(TextureHandle texture, Layer layer, Space space);

            // First member, so the context (bgfx itself) outlives every handle below.
            std::unique_ptr<BgfxContext> m_ownedContext;
            BgfxContext& m_context;
            bgfx::ViewId m_view = 0;
            bool m_offscreen = false;
            bgfx::FrameBufferHandle m_framebuffer = BGFX_INVALID_HANDLE;

            std::unique_ptr<BgfxProgram> m_program;
            bgfx::UniformHandle m_sampler = BGFX_INVALID_HANDLE;
            bgfx::TextureHandle m_white = BGFX_INVALID_HANDLE;
            bgfx::VertexLayout m_layout;
            std::string m_name;

            uint32_t m_width = 0;
            uint32_t m_height = 0;
            Camera2D m_camera;
            Color m_clear;
            bool m_inFrame = false;
            bool m_dropRestOfFrame = false;

            std::array<std::vector<Command>, LayerCount> m_layers{};
            std::vector<Vertex> m_meshVertices;
            std::vector<uint16_t> m_meshIndices;
            std::unordered_map<uint32_t, Texture> m_textures;
            uint32_t m_nextTexture = 1;
            std::vector<TextureHandle> m_pendingDestroy;

            bool m_warnedOutsideFrame = false;
            bool m_warnedInvalidTexture = false;
            bool m_warnedInvalidMesh = false;
            bool m_warnedTransient = false;
            bool m_warnedPolygon = false;
            bool m_warnedNullFont = false;

            TriangleMesh m_scratch;
    };

}

#endif //RENDERER_WRAPPER_BGFXRENDERER_HPP
