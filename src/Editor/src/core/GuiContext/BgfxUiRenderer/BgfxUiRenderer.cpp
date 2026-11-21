/*
    Created on: 17/11/2026
    Filename: BgfxUiRenderer.cpp
    Description: ImGui drawn by bgfx on the editor's window; owns the process's BgfxContext.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "BgfxUiRenderer.hpp"

#include "Renderer/BgfxContext/BgfxContext.hpp"
#include "Renderer/BgfxContext/BgfxTextureId.hpp"

#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/math.h>
#include <imgui_impl_sdl3.h>

// Precompiled for every renderer; a prefixed path, so bgfx's bundled ImGui headers never shadow the engine's.
#include <examples/common/imgui/fs_ocornut_imgui.bin.h>
#include <examples/common/imgui/vs_ocornut_imgui.bin.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace {

const bgfx::EmbeddedShader EmbeddedShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER_END(),
};

constexpr uint64_t DrawState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA
                             | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);

uint32_t toRgba(const ImVec4 &color)
{
    const auto channel = [](float value) { return static_cast<uint32_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f); };
    return (channel(color.x) << 24) | (channel(color.y) << 16) | (channel(color.z) << 8) | 0xFFu;
}

bgfx::TextureHandle textureOf(ImTextureID id)
{
    return bgfx::TextureHandle{static_cast<uint16_t>(gfx::bgfxTextureId::decode(static_cast<std::uintptr_t>(id)))};
}

} // namespace

struct BgfxUiRenderer::Device
{
    bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle sampler = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout layout;
};

BgfxUiRenderer::BgfxUiRenderer() : _device(std::make_unique<Device>()) {}

BgfxUiRenderer::~BgfxUiRenderer()
{
    if (_context) {
        destroyTextures();
        if (bgfx::isValid(_device->program))
            bgfx::destroy(_device->program);
        if (bgfx::isValid(_device->sampler))
            bgfx::destroy(_device->sampler);
        ImGuiIO &io = ImGui::GetIO();
        io.BackendRendererName = nullptr;
        io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
    }
    if (_imguiSdlBackend)
        ImGui_ImplSDL3_Shutdown();
    // Last: bgfx::shutdown, once every handle above is gone.
    _context.reset();
}

void BgfxUiRenderer::attach(SDL_Window *window)
{
    _window = window;
    if (!ImGui_ImplSDL3_InitForOther(window))
        throw std::runtime_error("ImGui SDL3 backend init failed");
    _imguiSdlBackend = true;

    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    _context = gfx::BgfxContext::create(window, static_cast<uint32_t>(std::max(width, 1)),
                                        static_cast<uint32_t>(std::max(height, 1)), true);
    if (!_context)
        throw std::runtime_error("bgfx could not start on the editor window");
    _name = std::string("bgfx / ") + _context->rendererName();

    const bgfx::RendererType::Enum type = bgfx::getRendererType();
    _device->program = bgfx::createProgram(bgfx::createEmbeddedShader(EmbeddedShaders, type, "vs_ocornut_imgui"),
                                           bgfx::createEmbeddedShader(EmbeddedShaders, type, "fs_ocornut_imgui"), true);
    _device->sampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
    _device->layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
    if (!bgfx::isValid(_device->program) || !bgfx::isValid(_device->sampler))
        throw std::runtime_error("bgfx could not create the ImGui program");

    ImGuiIO &io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_bgfx";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;
}

void BgfxUiRenderer::newFrame()
{
    ImGui_ImplSDL3_NewFrame();
}

void BgfxUiRenderer::updateTextures(ImDrawData *drawData)
{
    if (drawData->Textures == nullptr)
        return;
    for (ImTextureData *texture : *drawData->Textures) {
        switch (texture->Status) {
        case ImTextureStatus_WantCreate: {
            // ImGui's atlas is RGBA32 unless asked otherwise, which the editor never does. Created without data,
            // then filled: a texture created with data is immutable, and ImGui adds glyphs to it later.
            const auto width = static_cast<uint16_t>(texture->Width);
            const auto height = static_cast<uint16_t>(texture->Height);
            const bgfx::TextureHandle handle =
                bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, 0, nullptr);
            if (!bgfx::isValid(handle))
                break; // stays WantCreate: tried again next frame
            bgfx::updateTexture2D(handle, 0, 0, 0, 0, width, height,
                bgfx::copy(texture->GetPixels(), static_cast<uint32_t>(texture->GetSizeInBytes())));
            texture->SetTexID(static_cast<ImTextureID>(gfx::bgfxTextureId::encode(handle.idx)));
            texture->SetStatus(ImTextureStatus_OK);
            break;
        }
        case ImTextureStatus_WantUpdates: {
            const bgfx::TextureHandle handle = textureOf(texture->GetTexID());
            const int bytesPerPixel = texture->BytesPerPixel;
            for (const ImTextureRect &rect : texture->Updates) {
                const uint32_t rowBytes = static_cast<uint32_t>(rect.w * bytesPerPixel);
                const bgfx::Memory *pixels = bgfx::alloc(rowBytes * rect.h);
                for (int row = 0; row < rect.h; ++row)
                    std::memcpy(pixels->data + row * rowBytes, texture->GetPixelsAt(rect.x, rect.y + row), rowBytes);
                bgfx::updateTexture2D(handle, 0, 0, rect.x, rect.y, rect.w, rect.h, pixels);
            }
            texture->SetStatus(ImTextureStatus_OK);
            break;
        }
        case ImTextureStatus_WantDestroy:
            bgfx::destroy(textureOf(texture->GetTexID()));
            texture->SetTexID(ImTextureID_Invalid);
            texture->SetStatus(ImTextureStatus_Destroyed);
            break;
        default:
            break;
        }
    }
}

void BgfxUiRenderer::destroyTextures()
{
    for (ImTextureData *texture : ImGui::GetPlatformIO().Textures) {
        if (texture->RefCount == 1 && texture->GetTexID() != ImTextureID_Invalid) {
            bgfx::destroy(textureOf(texture->GetTexID()));
            texture->SetTexID(ImTextureID_Invalid);
            texture->SetStatus(ImTextureStatus_Destroyed);
        }
    }
}

void BgfxUiRenderer::render(const ImVec4 &clear)
{
    int pixelWidth = 0;
    int pixelHeight = 0;
    SDL_GetWindowSizeInPixels(_window, &pixelWidth, &pixelHeight);
    if (pixelWidth > 0 && pixelHeight > 0
        && (static_cast<uint32_t>(pixelWidth) != _context->width() || static_cast<uint32_t>(pixelHeight) != _context->height()))
        _context->reset(static_cast<uint32_t>(pixelWidth), static_cast<uint32_t>(pixelHeight), _context->vsync());

    ImDrawData *drawData = ImGui::GetDrawData();
    updateTextures(drawData);

    constexpr bgfx::ViewId view = gfx::BgfxContext::UiView;
    bgfx::setViewName(view, "ImGui");
    bgfx::setViewMode(view, bgfx::ViewMode::Sequential);
    bgfx::setViewClear(view, BGFX_CLEAR_COLOR, toRgba(clear));
    bgfx::setViewRect(view, 0, 0, static_cast<uint16_t>(_context->width()), static_cast<uint16_t>(_context->height()));
    {
        const ImVec2 pos = drawData->DisplayPos;
        const ImVec2 size = drawData->DisplaySize;
        float ortho[16];
        bx::mtxOrtho(ortho, pos.x, pos.x + size.x, pos.y + size.y, pos.y, 0.0f, 1000.0f, 0.0f,
                     bgfx::getCaps()->homogeneousDepth);
        bgfx::setViewTransform(view, nullptr, ortho);
    }
    bgfx::touch(view); // clear even when nothing is drawn

    const ImVec2 clipPos = drawData->DisplayPos;
    const ImVec2 clipScale = drawData->FramebufferScale;
    const float width = drawData->DisplaySize.x * clipScale.x;
    const float height = drawData->DisplaySize.y * clipScale.y;

    for (const ImDrawList *list : drawData->CmdLists) {
        const auto vertexCount = static_cast<uint32_t>(list->VtxBuffer.size());
        const auto indexCount = static_cast<uint32_t>(list->IdxBuffer.size());
        constexpr bool index32 = sizeof(ImDrawIdx) == 4;
        if (bgfx::getAvailTransientVertexBuffer(vertexCount, _device->layout) < vertexCount
            || bgfx::getAvailTransientIndexBuffer(indexCount, index32) < indexCount)
            break; // out of transient memory: the rest of this frame is dropped

        bgfx::TransientVertexBuffer vertices;
        bgfx::TransientIndexBuffer indices;
        bgfx::allocTransientVertexBuffer(&vertices, vertexCount, _device->layout);
        bgfx::allocTransientIndexBuffer(&indices, indexCount, index32);
        std::memcpy(vertices.data, list->VtxBuffer.Data, vertexCount * sizeof(ImDrawVert));
        std::memcpy(indices.data, list->IdxBuffer.Data, indexCount * sizeof(ImDrawIdx));

        for (const ImDrawCmd &cmd : list->CmdBuffer) {
            if (cmd.UserCallback != nullptr) {
                if (cmd.UserCallback != ImDrawCallback_ResetRenderState)
                    cmd.UserCallback(list, &cmd);
                continue;
            }
            if (cmd.ElemCount == 0)
                continue;

            const float x0 = std::max((cmd.ClipRect.x - clipPos.x) * clipScale.x, 0.0f);
            const float y0 = std::max((cmd.ClipRect.y - clipPos.y) * clipScale.y, 0.0f);
            const float x1 = std::min((cmd.ClipRect.z - clipPos.x) * clipScale.x, width);
            const float y1 = std::min((cmd.ClipRect.w - clipPos.y) * clipScale.y, height);
            if (x1 <= x0 || y1 <= y0)
                continue;

            bgfx::setScissor(static_cast<uint16_t>(x0), static_cast<uint16_t>(y0), static_cast<uint16_t>(x1 - x0),
                             static_cast<uint16_t>(y1 - y0));
            bgfx::setState(DrawState);
            bgfx::setTexture(0, _device->sampler, textureOf(cmd.GetTexID()));
            bgfx::setVertexBuffer(0, &vertices, cmd.VtxOffset, vertexCount - cmd.VtxOffset);
            bgfx::setIndexBuffer(&indices, cmd.IdxOffset, cmd.ElemCount);
            bgfx::submit(view, _device->program);
        }
    }

    _context->frame();
}

bool BgfxUiRenderer::setVSync(bool enabled)
{
    _context->reset(_context->width(), _context->height(), enabled);
    return true;
}

bool BgfxUiRenderer::isVSync() const
{
    return _context->vsync();
}

const char *BgfxUiRenderer::name() const
{
    return _name.c_str();
}
