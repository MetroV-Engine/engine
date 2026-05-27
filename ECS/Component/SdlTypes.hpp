#ifndef PROTO_SDLTYPES_HPP
#define PROTO_SDLTYPES_HPP

#include <cstdint>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_keyboard.h>

namespace Proto {

    struct Vec2f {
        float x = 0.f;
        float y = 0.f;
    };

    struct Vec2u {
        unsigned int x = 0;
        unsigned int y = 0;
    };

    struct Color {
        std::uint8_t r = 255;
        std::uint8_t g = 255;
        std::uint8_t b = 255;
        std::uint8_t a = 255;

        Color() = default;
        Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
            : r(r), g(g), b(b), a(a) {}

        static Color White()  { return {255, 255, 255, 255}; }
        static Color Black()  { return {0, 0, 0, 255}; }
        static Color Red()    { return {255, 0, 0, 255}; }
        static Color Green()  { return {0, 255, 0, 255}; }
        static Color Blue()   { return {0, 0, 255, 255}; }
    };

    using Key = SDL_Scancode;

    namespace Keys {
        constexpr Key A = SDL_SCANCODE_A;
        constexpr Key B = SDL_SCANCODE_B;
        constexpr Key C = SDL_SCANCODE_C;
        constexpr Key D = SDL_SCANCODE_D;
        constexpr Key E = SDL_SCANCODE_E;
        constexpr Key F = SDL_SCANCODE_F;
        constexpr Key G = SDL_SCANCODE_G;
        constexpr Key H = SDL_SCANCODE_H;
        constexpr Key I = SDL_SCANCODE_I;
        constexpr Key J = SDL_SCANCODE_J;
        constexpr Key K = SDL_SCANCODE_K;
        constexpr Key L = SDL_SCANCODE_L;
        constexpr Key M = SDL_SCANCODE_M;
        constexpr Key N = SDL_SCANCODE_N;
        constexpr Key O = SDL_SCANCODE_O;
        constexpr Key P = SDL_SCANCODE_P;
        constexpr Key Q = SDL_SCANCODE_Q;
        constexpr Key R = SDL_SCANCODE_R;
        constexpr Key S = SDL_SCANCODE_S;
        constexpr Key T = SDL_SCANCODE_T;
        constexpr Key U = SDL_SCANCODE_U;
        constexpr Key V = SDL_SCANCODE_V;
        constexpr Key W = SDL_SCANCODE_W;
        constexpr Key X = SDL_SCANCODE_X;
        constexpr Key Y = SDL_SCANCODE_Y;
        constexpr Key Z = SDL_SCANCODE_Z;
        constexpr Key Num0 = SDL_SCANCODE_0;
        constexpr Key Num1 = SDL_SCANCODE_1;
        constexpr Key Num2 = SDL_SCANCODE_2;
        constexpr Key Num3 = SDL_SCANCODE_3;
        constexpr Key Num4 = SDL_SCANCODE_4;
        constexpr Key Num5 = SDL_SCANCODE_5;
        constexpr Key Num6 = SDL_SCANCODE_6;
        constexpr Key Num7 = SDL_SCANCODE_7;
        constexpr Key Num8 = SDL_SCANCODE_8;
        constexpr Key Num9 = SDL_SCANCODE_9;
        constexpr Key Escape = SDL_SCANCODE_ESCAPE;
        constexpr Key LControl = SDL_SCANCODE_LCTRL;
        constexpr Key LShift = SDL_SCANCODE_LSHIFT;
        constexpr Key LAlt = SDL_SCANCODE_LALT;
        constexpr Key LSystem = SDL_SCANCODE_LGUI;
        constexpr Key RControl = SDL_SCANCODE_RCTRL;
        constexpr Key RShift = SDL_SCANCODE_RSHIFT;
        constexpr Key RAlt = SDL_SCANCODE_RALT;
        constexpr Key RSystem = SDL_SCANCODE_RGUI;
        constexpr Key Menu = SDL_SCANCODE_MENU;
        constexpr Key LBracket = SDL_SCANCODE_LEFTBRACKET;
        constexpr Key RBracket = SDL_SCANCODE_RIGHTBRACKET;
        constexpr Key Semicolon = SDL_SCANCODE_SEMICOLON;
        constexpr Key Comma = SDL_SCANCODE_COMMA;
        constexpr Key Period = SDL_SCANCODE_PERIOD;
        constexpr Key Apostrophe = SDL_SCANCODE_APOSTROPHE;
        constexpr Key Slash = SDL_SCANCODE_SLASH;
        constexpr Key Backslash = SDL_SCANCODE_BACKSLASH;
        constexpr Key Grave = SDL_SCANCODE_GRAVE;
        constexpr Key Equal = SDL_SCANCODE_EQUALS;
        constexpr Key Hyphen = SDL_SCANCODE_MINUS;
        constexpr Key Space = SDL_SCANCODE_SPACE;
        constexpr Key Enter = SDL_SCANCODE_RETURN;
        constexpr Key Backspace = SDL_SCANCODE_BACKSPACE;
        constexpr Key Tab = SDL_SCANCODE_TAB;
        constexpr Key PageUp = SDL_SCANCODE_PAGEUP;
        constexpr Key PageDown = SDL_SCANCODE_PAGEDOWN;
        constexpr Key End = SDL_SCANCODE_END;
        constexpr Key Home = SDL_SCANCODE_HOME;
        constexpr Key Insert = SDL_SCANCODE_INSERT;
        constexpr Key Delete = SDL_SCANCODE_DELETE;
        constexpr Key Add = SDL_SCANCODE_KP_PLUS;
        constexpr Key Subtract = SDL_SCANCODE_KP_MINUS;
        constexpr Key Multiply = SDL_SCANCODE_KP_MULTIPLY;
        constexpr Key Divide = SDL_SCANCODE_KP_DIVIDE;
        constexpr Key Left = SDL_SCANCODE_LEFT;
        constexpr Key Right = SDL_SCANCODE_RIGHT;
        constexpr Key Up = SDL_SCANCODE_UP;
        constexpr Key Down = SDL_SCANCODE_DOWN;
        constexpr Key Numpad0 = SDL_SCANCODE_KP_0;
        constexpr Key Numpad1 = SDL_SCANCODE_KP_1;
        constexpr Key Numpad2 = SDL_SCANCODE_KP_2;
        constexpr Key Numpad3 = SDL_SCANCODE_KP_3;
        constexpr Key Numpad4 = SDL_SCANCODE_KP_4;
        constexpr Key Numpad5 = SDL_SCANCODE_KP_5;
        constexpr Key Numpad6 = SDL_SCANCODE_KP_6;
        constexpr Key Numpad7 = SDL_SCANCODE_KP_7;
        constexpr Key Numpad8 = SDL_SCANCODE_KP_8;
        constexpr Key Numpad9 = SDL_SCANCODE_KP_9;
        constexpr Key F1 = SDL_SCANCODE_F1;
        constexpr Key F2 = SDL_SCANCODE_F2;
        constexpr Key F3 = SDL_SCANCODE_F3;
        constexpr Key F4 = SDL_SCANCODE_F4;
        constexpr Key F5 = SDL_SCANCODE_F5;
        constexpr Key F6 = SDL_SCANCODE_F6;
        constexpr Key F7 = SDL_SCANCODE_F7;
        constexpr Key F8 = SDL_SCANCODE_F8;
        constexpr Key F9 = SDL_SCANCODE_F9;
        constexpr Key F10 = SDL_SCANCODE_F10;
        constexpr Key F11 = SDL_SCANCODE_F11;
        constexpr Key F12 = SDL_SCANCODE_F12;
        constexpr Key F13 = SDL_SCANCODE_F13;
        constexpr Key F14 = SDL_SCANCODE_F14;
        constexpr Key F15 = SDL_SCANCODE_F15;
    }

    // NOTE: This function reads SDL's global keyboard state directly.
    // In multi-threaded architectures where input is polled on the main thread
    // and gameplay runs on a separate game thread, this may be unsafe.
    // Consider using a thread-safe input state snapshot system instead.
    inline bool isKeyPressed(Key key) {
        const bool *state = SDL_GetKeyboardState(nullptr);
        return state[key];
    }

} // Proto

#endif //PROTO_SDLTYPES_HPP
