/*
    Created on: 21/11/2026
    Filename: UiBackendOption.cpp
    Description: The --backend=sdl|bgfx command-line option shared by the apps that open the editor.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "core/GuiContext/UiBackendOption.hpp"

std::optional<UiBackend> parseUiBackendOption(std::string_view arg)
{
    if (arg == "--backend=sdl")
        return UiBackend::Sdl;
    if (arg == "--backend=bgfx")
        return UiBackend::Bgfx;
    return std::nullopt;
}

bool isUiBackendBuilt(UiBackend backend)
{
#if ENGINE_RENDERER_BGFX
    (void)backend;
    return true;
#else
    return backend != UiBackend::Bgfx;
#endif
}
