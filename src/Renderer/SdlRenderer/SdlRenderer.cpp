/*
    Created on: 9/22/26
    Filename: SdlRenderer.cpp
    Description: SDL3 backend: the interim renderer the UI team builds against.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/SdlRenderer/SdlRenderer.hpp"

#include "Renderer/Font/Font.hpp"
#include "Renderer/ShapeBuilder/ShapeBuilder.hpp"
#include "Renderer/Transform/Transform.hpp"

#include <SDL3/SDL.h>

#include <iostream>
#include <numbers>

namespace gfx {

    namespace {
        SDL_FRect toSdl(const Rect& rect)
        {
            return SDL_FRect{ rect.x, rect.y, rect.w, rect.h };
        }

        double toDegrees(const float radians)
        {
            return static_cast<double>(radians) * 180.0 / std::numbers::pi;
        }

        SDL_FColor toSdlColor(const Vertex& vertex)
        {
            return SDL_FColor{ static_cast<float>(vertex.r) / 255.0f, static_cast<float>(vertex.g) / 255.0f,
                               static_cast<float>(vertex.b) / 255.0f, static_cast<float>(vertex.a) / 255.0f };
        }
    }

    std::unique_ptr<SdlRenderer> SdlRenderer::create(SDL_Window* window, const uint32_t width, const uint32_t height)
    {
        if (window == nullptr) {
            std::cerr << "[SdlRenderer] ERROR: null window\n";
            return nullptr;
        }

        SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
        if (renderer == nullptr) {
            std::cerr << "[SdlRenderer] ERROR: SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
            return nullptr;
        }

        // Rect alpha is meaningless without this; SDL defaults draws to no blending.
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        // bgfx clamps UVs; the SDL default would tile coordinates outside 0..1.
        SDL_SetRenderTextureAddressMode(renderer, SDL_TEXTURE_ADDRESS_CLAMP, SDL_TEXTURE_ADDRESS_CLAMP);

        return std::unique_ptr<SdlRenderer>(new SdlRenderer(renderer, width, height));
    }

    SdlRenderer::SdlRenderer(SDL_Renderer* renderer, const uint32_t width, const uint32_t height)
        : m_renderer(renderer)
        , m_width(width)
        , m_height(height)
    {
        const char* driver = SDL_GetRendererName(m_renderer);
        m_name = std::string("SDL3 / ") + (driver != nullptr ? driver : "unknown");

        // World coordinates then match screen pixels, as the ECS prototype expects.
        m_camera.center = Vec2{ static_cast<float>(width) * 0.5f, static_cast<float>(height) * 0.5f };

        std::cerr << "[SdlRenderer] ready: " << m_name << " " << width << "x" << height << "\n";
    }

    SdlRenderer::~SdlRenderer()
    {
        for (const auto& [id, texture] : m_textures) {
            SDL_DestroyTexture(texture);
        }
        m_textures.clear();

        if (m_renderer != nullptr) {
            SDL_DestroyRenderer(m_renderer);
            m_renderer = nullptr;
        }
    }

    void SdlRenderer::resize(const uint32_t width, const uint32_t height)
    {
        if (width == 0 || height == 0) {
            return;
        }

        m_width = width;
        m_height = height;
    }

    void SdlRenderer::beginFrame(const Color clear)
    {
        if (m_inFrame) {
            std::cerr << "[SdlRenderer] WARN: beginFrame() called twice; ignoring\n";
            return;
        }

        for (auto& layer : m_layers) {
            layer.clear();
        }
        m_meshVertices.clear();
        m_meshIndices.clear();

        SDL_SetRenderDrawColor(m_renderer, clear.r, clear.g, clear.b, clear.a);
        SDL_RenderClear(m_renderer);
        m_inFrame = true;
    }

    void SdlRenderer::endFrame()
    {
        if (!m_inFrame) {
            std::cerr << "[SdlRenderer] WARN: endFrame() without beginFrame(); ignoring\n";
            return;
        }

        for (const auto& layer : m_layers) {
            for (const Command& command : layer) {
                flush(command);
            }
        }

        SDL_RenderPresent(m_renderer);
        m_inFrame = false;
        releasePending();
    }

    void SdlRenderer::setCamera(const Camera2D& camera)
    {
        m_camera = camera;
    }

    TextureHandle SdlRenderer::createTexture(const uint8_t* rgba, const uint32_t width, const uint32_t height)
    {
        if (rgba == nullptr || width == 0 || height == 0) {
            std::cerr << "[SdlRenderer] ERROR: createTexture called with empty pixels\n";
            return TextureHandle::Invalid;
        }

        SDL_Texture* texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STATIC, static_cast<int>(width), static_cast<int>(height));
        if (texture == nullptr) {
            std::cerr << "[SdlRenderer] ERROR: SDL_CreateTexture failed: " << SDL_GetError() << "\n";
            return TextureHandle::Invalid;
        }

        // UpdateTexture copies, so the caller keeps ownership of its pixels.
        if (!SDL_UpdateTexture(texture, nullptr, rgba, static_cast<int>(width * 4))) {
            std::cerr << "[SdlRenderer] ERROR: SDL_UpdateTexture failed: " << SDL_GetError() << "\n";
            SDL_DestroyTexture(texture);
            return TextureHandle::Invalid;
        }

        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

        const uint32_t id = m_nextTexture++;
        m_textures.emplace(id, texture);

        return static_cast<TextureHandle>(id);
    }

    void SdlRenderer::destroyTexture(const TextureHandle texture)
    {
        if (find(texture) == nullptr) {
            if (!m_warnedInvalidTexture) {
                std::cerr << "[SdlRenderer] WARN: destroyTexture on an unknown handle\n";
                m_warnedInvalidTexture = true;
            }
            return;
        }

        // Draws are queued until endFrame(), so the pixels must outlive the frame that uses them.
        if (m_inFrame) {
            m_pendingDestroy.push_back(texture);
            return;
        }

        const auto id = static_cast<uint32_t>(texture);
        SDL_DestroyTexture(m_textures[id]);
        m_textures.erase(id);
    }

    Vec2 SdlRenderer::textureSize(const TextureHandle texture) const
    {
        SDL_Texture* found = find(texture);
        if (found == nullptr) {
            return Vec2{ 0.0f, 0.0f };
        }

        Vec2 size;
        SDL_GetTextureSize(found, &size.x, &size.y);

        return size;
    }

    void SdlRenderer::drawRect(const RectDraw& draw)
    {
        Command command;
        command.kind = Command::Kind::Rect;
        command.dest = draw.dest;
        command.color = draw.color;
        command.space = draw.space;

        submit(command, draw.layer);
    }

    void SdlRenderer::drawSprite(const SpriteDraw& draw)
    {
        if (find(draw.texture) == nullptr) {
            if (!m_warnedInvalidTexture) {
                std::cerr << "[SdlRenderer] WARN: drawSprite with an invalid texture handle\n";
                m_warnedInvalidTexture = true;
            }
            return;
        }

        Command command;
        command.kind = Command::Kind::Sprite;
        command.texture = draw.texture;
        command.source = draw.source;
        command.dest = draw.dest;
        command.rotation = draw.rotation;
        command.flipX = draw.flipX;
        command.color = draw.tint;
        command.space = draw.space;

        submit(command, draw.layer);
    }

    void SdlRenderer::drawMesh(const MeshDraw& draw)
    {
        if (draw.vertices.empty() || draw.indices.empty()) {
            return;
        }

        // Before the arenas: beginFrame() is the only thing that clears them.
        if (!inFrameOrWarn()) {
            return;
        }

        const bool knownTexture = draw.texture == TextureHandle::Invalid || find(draw.texture) != nullptr;
        if (!isValidMesh(draw.vertices, draw.indices) || !knownTexture) {
            if (!m_warnedInvalidMesh) {
                std::cerr << "[SdlRenderer] WARN: drawMesh with malformed geometry or an unknown texture; ignoring\n";
                m_warnedInvalidMesh = true;
            }
            return;
        }

        Command command;
        command.kind = Command::Kind::Mesh;
        command.texture = draw.texture;
        command.space = draw.space;
        command.firstVertex = static_cast<uint32_t>(m_meshVertices.size());
        command.vertexCount = static_cast<uint32_t>(draw.vertices.size());
        command.firstIndex = static_cast<uint32_t>(m_meshIndices.size());
        command.indexCount = static_cast<uint32_t>(draw.indices.size());

        m_meshVertices.insert(m_meshVertices.end(), draw.vertices.begin(), draw.vertices.end());
        m_meshIndices.insert(m_meshIndices.end(), draw.indices.begin(), draw.indices.end());

        submit(command, draw.layer);
    }

    void SdlRenderer::drawLine(const LineDraw& draw)
    {
        m_scratch.clear();
        appendLine(m_scratch, draw);
        submitScratch(TextureHandle::Invalid, draw.layer, draw.space);
    }

    void SdlRenderer::drawTriangle(const TriangleDraw& draw)
    {
        m_scratch.clear();
        appendTriangle(m_scratch, draw);
        submitScratch(TextureHandle::Invalid, draw.layer, draw.space);
    }

    void SdlRenderer::drawQuad(const QuadDraw& draw)
    {
        m_scratch.clear();
        appendQuad(m_scratch, draw);
        submitScratch(TextureHandle::Invalid, draw.layer, draw.space);
    }

    void SdlRenderer::drawCircle(const CircleDraw& draw)
    {
        m_scratch.clear();
        appendCircle(m_scratch, draw);
        submitScratch(TextureHandle::Invalid, draw.layer, draw.space);
    }

    void SdlRenderer::drawPolygon(const PolygonDraw& draw)
    {
        if (draw.points.size() < 3) {
            return;
        }

        m_scratch.clear();
        if (!appendPolygon(m_scratch, draw)) {
            if (!m_warnedPolygon) {
                std::cerr << "[SdlRenderer] WARN: drawPolygon could not triangulate its points; ignoring\n";
                m_warnedPolygon = true;
            }
            return;
        }

        submitScratch(TextureHandle::Invalid, draw.layer, draw.space);
    }

    void SdlRenderer::drawText(const TextDraw& draw)
    {
        if (draw.font == nullptr) {
            if (!m_warnedNullFont) {
                std::cerr << "[SdlRenderer] WARN: drawText called with a null font; ignoring\n";
                m_warnedNullFont = true;
            }
            return;
        }

        m_scratch.clear();
        draw.font->appendText(m_scratch, draw);
        submitScratch(draw.font->texture(), draw.layer, draw.space);
    }

    void SdlRenderer::submitScratch(const TextureHandle texture, const Layer layer, const Space space)
    {
        if (m_scratch.vertices.empty()) {
            return;
        }

        drawMesh(MeshDraw{ m_scratch.vertices, m_scratch.indices, texture, layer, space });
    }

    const char* SdlRenderer::backendName() const
    {
        return m_name.c_str();
    }

    bool SdlRenderer::inFrameOrWarn()
    {
        if (m_inFrame) {
            return true;
        }

        if (!m_warnedOutsideFrame) {
            std::cerr << "[SdlRenderer] WARN: draw outside beginFrame()/endFrame(); ignoring\n";
            m_warnedOutsideFrame = true;
        }
        return false;
    }

    void SdlRenderer::submit(const Command& command, const Layer layer)
    {
        if (!inFrameOrWarn()) {
            return;
        }

        m_layers[static_cast<size_t>(layerSlot(layer))].push_back(command);
    }

    void SdlRenderer::flushMesh(const Command& command)
    {
        SDL_Texture* texture = nullptr;
        if (command.texture != TextureHandle::Invalid) {
            texture = find(command.texture);
            if (texture == nullptr) {
                return;
            }
        }

        const Vec2 viewport{ static_cast<float>(m_width), static_cast<float>(m_height) };

        m_geometryVertices.resize(command.vertexCount);
        for (uint32_t i = 0; i < command.vertexCount; ++i) {
            const Vertex& vertex = m_meshVertices[command.firstVertex + i];
            const Vec2 position{ vertex.x, vertex.y };
            const Vec2 screen = command.space == Space::World
                ? worldToScreen(m_camera, position, viewport)
                : position;

            m_geometryVertices[i] = SDL_Vertex{ SDL_FPoint{ screen.x, screen.y }, toSdlColor(vertex),
                                                SDL_FPoint{ vertex.u, vertex.v } };
        }

        m_geometryIndices.resize(command.indexCount);
        for (uint32_t i = 0; i < command.indexCount; ++i) {
            m_geometryIndices[i] = m_meshIndices[command.firstIndex + i];
        }

        SDL_RenderGeometry(m_renderer, texture, m_geometryVertices.data(), static_cast<int>(command.vertexCount),
            m_geometryIndices.data(), static_cast<int>(command.indexCount));
    }

    void SdlRenderer::flush(const Command& command)
    {
        if (command.kind == Command::Kind::Mesh) {
            flushMesh(command);
            return;
        }

        const Vec2 viewport{ static_cast<float>(m_width), static_cast<float>(m_height) };
        const Rect dest = command.space == Space::World
            ? worldToScreen(m_camera, command.dest, viewport)
            : command.dest;
        const SDL_FRect destination = toSdl(dest);

        if (command.kind == Command::Kind::Rect) {
            SDL_SetRenderDrawColor(m_renderer, command.color.r, command.color.g, command.color.b, command.color.a);
            SDL_RenderFillRect(m_renderer, &destination);
            return;
        }

        SDL_Texture* texture = find(command.texture);
        if (texture == nullptr) {
            return;
        }

        SDL_SetTextureColorMod(texture, command.color.r, command.color.g, command.color.b);
        SDL_SetTextureAlphaMod(texture, command.color.a);

        const SDL_FRect source = toSdl(command.source);
        const bool wholeTexture = command.source.w <= 0.0f || command.source.h <= 0.0f;

        SDL_RenderTextureRotated(m_renderer, texture, wholeTexture ? nullptr : &source, &destination,
            toDegrees(command.rotation), nullptr, command.flipX ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    }

    SDL_Texture* SdlRenderer::find(const TextureHandle texture) const
    {
        const auto found = m_textures.find(static_cast<uint32_t>(texture));

        return found != m_textures.end() ? found->second : nullptr;
    }

    void SdlRenderer::releasePending()
    {
        for (const TextureHandle texture : m_pendingDestroy) {
            const auto id = static_cast<uint32_t>(texture);
            const auto found = m_textures.find(id);
            if (found != m_textures.end()) {
                SDL_DestroyTexture(found->second);
                m_textures.erase(found);
            }
        }

        m_pendingDestroy.clear();
    }

}
