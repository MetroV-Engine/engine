/*
    Created on: 21/11/2026
    Filename: UiBackendOption.hpp
    Description: The --backend=sdl|bgfx command-line option shared by the apps that open the editor.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "core/GuiContext/UiRenderer.hpp"

#include <optional>
#include <string_view>

// The backend "--backend=sdl" or "--backend=bgfx" names; nullopt for any other argument.
std::optional<UiBackend> parseUiBackendOption(std::string_view arg);

// False for bgfx when this build has ENGINE_RENDERER_BGFX=OFF.
bool isUiBackendBuilt(UiBackend backend);

// The option's lines for a usage text.
inline constexpr const char *UiBackendOptionHelp =
    "  --backend=sdl    Draw with SDL3 (the default)\n"
    "  --backend=bgfx   Draw with bgfx (needs a build with ENGINE_RENDERER_BGFX=ON)\n";
