/*
    Filename: ScaleProvider.hpp
    Description: Tracks the editor window size and the resulting UI scale factor.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_PROVIDERS_SCALEPROVIDER_HPP
#define PROTO_UI_PROVIDERS_SCALEPROVIDER_HPP

#include "SdlTypes.hpp"
#include <SDL3/SDL.h>

namespace Editor::UI::Providers {

#ifndef BASE_RESOLUTION_W
#define BASE_RESOLUTION_W 1920
#endif
#ifndef BASE_RESOLUTION_H
#define BASE_RESOLUTION_H 1080
#endif

    class ScaleProvider {
    public:
        static ScaleProvider &getInstance();

        [[nodiscard]] Proto::Vec2f getScale() const;
        [[nodiscard]] Proto::Vec2u getWinSize() const;

        void setScale(Proto::Vec2f scale);
        void setWinSize(Proto::Vec2u windowSize);
        void initWinSize();

    private:
        ScaleProvider();
        ~ScaleProvider() = default;

        Proto::Vec2u _winSize;
        Proto::Vec2f _scale;
    };

    inline ScaleProvider &useScale() { return ScaleProvider::getInstance(); }

}

#endif
