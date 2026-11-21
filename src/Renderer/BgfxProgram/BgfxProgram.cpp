/*
    Created on: 10/4/26
    Filename: BgfxProgram.cpp
    Description: RAII sprite shader program, embedded for the API bgfx picked.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "Renderer/BgfxProgram/BgfxProgram.hpp"

#include <cstdint>
#include <iostream>
#include <span>

// Generated at build time by shaderc, for the profiles of this host (see cmake/Bgfx.cmake).
#if __has_include(<generated/shaders/glsl/vs_sprite.sc.bin.h>)
#include <generated/shaders/glsl/vs_sprite.sc.bin.h>
#include <generated/shaders/glsl/fs_sprite.sc.bin.h>
#define ENGINE_SHADERS_GLSL 1
#endif
#if __has_include(<generated/shaders/spirv/vs_sprite.sc.bin.h>)
#include <generated/shaders/spirv/vs_sprite.sc.bin.h>
#include <generated/shaders/spirv/fs_sprite.sc.bin.h>
#define ENGINE_SHADERS_SPIRV 1
#endif
#if __has_include(<generated/shaders/dxbc/vs_sprite.sc.bin.h>)
#include <generated/shaders/dxbc/vs_sprite.sc.bin.h>
#include <generated/shaders/dxbc/fs_sprite.sc.bin.h>
#define ENGINE_SHADERS_DXBC 1
#endif
#if __has_include(<generated/shaders/metal/vs_sprite.sc.bin.h>)
#include <generated/shaders/metal/vs_sprite.sc.bin.h>
#include <generated/shaders/metal/fs_sprite.sc.bin.h>
#define ENGINE_SHADERS_METAL 1
#endif

namespace gfx {

    namespace {
        // A span built from the generated arrays keeps their size; a raw pointer would lose it.
        struct ShaderVariants {
            std::span<const uint8_t> glsl;
            std::span<const uint8_t> spirv;
            std::span<const uint8_t> dxbc;
            std::span<const uint8_t> metal;
        };

        // Not BGFX_EMBEDDED_SHADER: it references every profile's symbols, and each host compiles only its own.
        bgfx::ShaderHandle createEmbeddedShader(const ShaderVariants& variants, const char* name)
        {
            std::span<const uint8_t> blob;
            switch (bgfx::getRendererType()) {
                case bgfx::RendererType::OpenGL:
                    blob = variants.glsl;
                    break;
                case bgfx::RendererType::Vulkan:
                    blob = variants.spirv;
                    break;
                case bgfx::RendererType::Direct3D11:
                    blob = variants.dxbc;
                    break;
                case bgfx::RendererType::Metal:
                    blob = variants.metal;
                    break;
                default:
                    break;
            }

        if (blob.empty()) {
                std::cerr << "[BgfxProgram] ERROR: no embedded '" << name << "' shader for "
                          << bgfx::getRendererName(bgfx::getRendererType()) << "\n";
                return BGFX_INVALID_HANDLE;
            }

            const bgfx::ShaderHandle handle =
                bgfx::createShader(bgfx::makeRef(blob.data(), static_cast<uint32_t>(blob.size())));
            if (bgfx::isValid(handle)) {
                bgfx::setName(handle, name);
            }

            return handle;
        }
    }

    BgfxProgram::BgfxProgram()
    {
        ShaderVariants vertexVariants;
        ShaderVariants fragmentVariants;
#if ENGINE_SHADERS_GLSL
        vertexVariants.glsl = vs_sprite_glsl;
        fragmentVariants.glsl = fs_sprite_glsl;
#endif
#if ENGINE_SHADERS_SPIRV
        vertexVariants.spirv = vs_sprite_spv;
        fragmentVariants.spirv = fs_sprite_spv;
#endif
#if ENGINE_SHADERS_DXBC
        vertexVariants.dxbc = vs_sprite_dxbc;
        fragmentVariants.dxbc = fs_sprite_dxbc;
#endif
#if ENGINE_SHADERS_METAL
        vertexVariants.metal = vs_sprite_mtl;
        fragmentVariants.metal = fs_sprite_mtl;
#endif
        const bgfx::ShaderHandle vertex = createEmbeddedShader(vertexVariants, "vs_sprite");
        const bgfx::ShaderHandle fragment = createEmbeddedShader(fragmentVariants, "fs_sprite");

        if (!bgfx::isValid(vertex) || !bgfx::isValid(fragment)) {
            if (bgfx::isValid(vertex)) {
                bgfx::destroy(vertex);
            }
            if (bgfx::isValid(fragment)) {
                bgfx::destroy(fragment);
            }
            return;
        }

        m_program = bgfx::createProgram(vertex, fragment, true /* destroy shaders with program */);
        if (!bgfx::isValid(m_program)) {
            std::cerr << "[BgfxProgram] ERROR: bgfx::createProgram failed\n";
        }
    }

    BgfxProgram::~BgfxProgram()
    {
        if (bgfx::isValid(m_program)) {
            bgfx::destroy(m_program);
        }
    }

    bool BgfxProgram::valid() const
    {
        return bgfx::isValid(m_program);
    }

    bgfx::ProgramHandle BgfxProgram::handle() const
    {
        return m_program;
    }

}
