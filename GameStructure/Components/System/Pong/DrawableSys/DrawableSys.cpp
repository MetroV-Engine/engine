#include "DrawableSys.hpp"
#include "Entity/Base/Bounds/Bounds.hpp"
#include "Entity/Base/Position/Position.hpp"
#include "ProjectContext.hpp"

#include <bgfx/bgfx.h>
#include <fstream>
#include <vector>
#include <string>

struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;
};

static bgfx::ShaderHandle loadShaderFromFile(const std::string &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return BGFX_INVALID_HANDLE;

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    const bgfx::Memory *mem = bgfx::alloc(static_cast<uint32_t>(size) + 1);
    file.read(reinterpret_cast<char *>(mem->data), size);
    mem->data[mem->size - 1] = '\0';

    return bgfx::createShader(mem);
}

static std::string getShaderSubdir()
{
    switch (bgfx::getRendererType()) {
        case bgfx::RendererType::Metal: return "metal";
        case bgfx::RendererType::OpenGL: return "glsl";
        case bgfx::RendererType::OpenGLES: return "essl";
        case bgfx::RendererType::Vulkan: return "spirv";
        case bgfx::RendererType::Direct3D11: return "dxbc";
        case bgfx::RendererType::Direct3D12: return "dxil";
        default: return "spirv";
    }
}

namespace ECS::System {

    DrawableSys::DrawableSys()
    {
        _layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
    }

    DrawableSys::~DrawableSys()
    {
    }

    void DrawableSys::update(registry &r, float deltaTime, SDL_Window *win)
    {
        if (!bgfx::isValid(_program)) {
            std::string dir = std::string(SHADER_OUTPUT_DIR) + "/" + getShaderSubdir() + "/";
            bgfx::ShaderHandle vsh = loadShaderFromFile(dir + "vs_color.sc.bin");
            bgfx::ShaderHandle fsh = loadShaderFromFile(dir + "fs_color.sc.bin");
            if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh))
                return;
            _program = bgfx::createProgram(vsh, fsh, true);
            if (!bgfx::isValid(_program))
                return;
        }

        auto *pos = r.get_if<Entity::Position>();
        auto *bounds = r.get_if<Entity::Bounds>();
        if (!pos || !bounds) return;

        float screenW = Editor::ProjectContext::getInstance().screenWidth();
        float screenH = Editor::ProjectContext::getInstance().screenHeight();

        for (auto [p, b, entity] : zipper(*pos, *bounds)) {
            float x0 = (p._x / screenW) * 2.0f - 1.0f;
            float y0 = 1.0f - (p._y / screenH) * 2.0f;
            float x1 = ((p._x + b._width) / screenW) * 2.0f - 1.0f;
            float y1 = 1.0f - ((p._y + b._height) / screenH) * 2.0f;

            uint32_t abgr = (uint32_t(b._color.a) << 24)
                          | (uint32_t(b._color.b) << 16)
                          | (uint32_t(b._color.g) << 8)
                          | uint32_t(b._color.r);

            bgfx::TransientVertexBuffer tvb;
            bgfx::TransientIndexBuffer tib;

            if (!bgfx::allocTransientBuffers(&tvb, _layout, 4, &tib, 6))
                continue;

            auto *verts = reinterpret_cast<PosColorVertex *>(tvb.data);
            verts[0] = {x0, y0, 0.0f, abgr};
            verts[1] = {x1, y0, 0.0f, abgr};
            verts[2] = {x1, y1, 0.0f, abgr};
            verts[3] = {x0, y1, 0.0f, abgr};

            auto *indices = reinterpret_cast<uint16_t *>(tib.data);
            indices[0] = 0; indices[1] = 1; indices[2] = 2;
            indices[3] = 0; indices[4] = 2; indices[5] = 3;

            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setIndexBuffer(&tib);
            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
            bgfx::submit(0, _program);
        }
    }
}
