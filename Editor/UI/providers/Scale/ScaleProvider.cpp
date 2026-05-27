#include "ScaleProvider.hpp"

namespace Editor::UI::Providers {

    ScaleProvider &ScaleProvider::getInstance()
    {
        static ScaleProvider instance;
        return instance;
    }

    Proto::Vec2f ScaleProvider::getScale() const { return _scale; }
    Proto::Vec2u ScaleProvider::getWinSize() const { return _winSize; }
    void ScaleProvider::setScale(const Proto::Vec2f scale) { _scale = scale; }
    void ScaleProvider::setWinSize(const Proto::Vec2u windowSize) { _winSize = windowSize; }

    void ScaleProvider::initWinSize()
    {
        const SDL_DisplayMode *dm = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
        if (dm) {
            _winSize = {static_cast<unsigned>(dm->w), static_cast<unsigned>(dm->h)};
        } else {
            _winSize = {BASE_RESOLUTION_W, BASE_RESOLUTION_H};
        }
        _scale.x = static_cast<float>(_winSize.x) / BASE_RESOLUTION_W;
        _scale.y = static_cast<float>(_winSize.y) / BASE_RESOLUTION_H;
    }

    ScaleProvider::ScaleProvider()
        : _winSize{BASE_RESOLUTION_W, BASE_RESOLUTION_H}, _scale{1.0f, 1.0f}
    {
    }

}
