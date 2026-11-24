/*
    Created on: 11/17/26
    Filename: BasePath.hpp
    Description: Paths next to the executable, where the build copies assets.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef RENDERER_WRAPPER_BASEPATH_HPP
#define RENDERER_WRAPPER_BASEPATH_HPP

#include <string>
#include <string_view>

namespace tools {

    // The working directory is not the binary's (CLion, ctest): assets are resolved from the binary instead.
    std::string pathNextToExecutable(std::string_view relative);

}

#endif //RENDERER_WRAPPER_BASEPATH_HPP
