#ifndef PROTO_VALUERENDERER_HPP
#define PROTO_VALUERENDERER_HPP

#include <unordered_map>
#include <functional>
#include <map>
#include "SdlTypes.hpp"
#include "Entity/Base/Controlable/Controlable.hpp"
#include "Reflection/FieldDescriptor.hpp"

namespace Editor {
    static constexpr const char *keyNames[] = {
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
        "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
        "Num0", "Num1", "Num2", "Num3", "Num4", "Num5", "Num6", "Num7", "Num8", "Num9",
        "Escape", "LControl", "LShift", "LAlt", "LSystem", "RControl", "RShift", "RAlt", "RSystem",
        "Menu", "LBracket", "RBracket", "Semicolon", "Comma", "Period", "Apostrophe", "Slash",
        "Backslash", "Grave", "Equal", "Hyphen", "Space", "Enter", "Backspace", "Tab",
        "PageUp", "PageDown", "End", "Home", "Insert", "Delete",
        "Add", "Subtract", "Multiply", "Divide",
        "Left", "Right", "Up", "Down",
        "Numpad0", "Numpad1", "Numpad2", "Numpad3", "Numpad4", "Numpad5", "Numpad6", "Numpad7", "Numpad8",
        "Numpad9",
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15"
    };

    static constexpr Proto::Key keyValues[] = {
        Proto::Keys::A, Proto::Keys::B, Proto::Keys::C, Proto::Keys::D, Proto::Keys::E, Proto::Keys::F,
        Proto::Keys::G, Proto::Keys::H, Proto::Keys::I, Proto::Keys::J, Proto::Keys::K, Proto::Keys::L,
        Proto::Keys::M, Proto::Keys::N, Proto::Keys::O, Proto::Keys::P, Proto::Keys::Q, Proto::Keys::R,
        Proto::Keys::S, Proto::Keys::T, Proto::Keys::U, Proto::Keys::V, Proto::Keys::W, Proto::Keys::X,
        Proto::Keys::Y, Proto::Keys::Z,
        Proto::Keys::Num0, Proto::Keys::Num1, Proto::Keys::Num2, Proto::Keys::Num3, Proto::Keys::Num4,
        Proto::Keys::Num5, Proto::Keys::Num6, Proto::Keys::Num7, Proto::Keys::Num8, Proto::Keys::Num9,
        Proto::Keys::Escape, Proto::Keys::LControl, Proto::Keys::LShift, Proto::Keys::LAlt, Proto::Keys::LSystem,
        Proto::Keys::RControl, Proto::Keys::RShift, Proto::Keys::RAlt, Proto::Keys::RSystem,
        Proto::Keys::Menu, Proto::Keys::LBracket, Proto::Keys::RBracket, Proto::Keys::Semicolon, Proto::Keys::Comma,
        Proto::Keys::Period, Proto::Keys::Apostrophe, Proto::Keys::Slash,
        Proto::Keys::Backslash, Proto::Keys::Grave, Proto::Keys::Equal, Proto::Keys::Hyphen, Proto::Keys::Space,
        Proto::Keys::Enter, Proto::Keys::Backspace, Proto::Keys::Tab,
        Proto::Keys::PageUp, Proto::Keys::PageDown, Proto::Keys::End, Proto::Keys::Home,
        Proto::Keys::Insert, Proto::Keys::Delete,
        Proto::Keys::Add, Proto::Keys::Subtract, Proto::Keys::Multiply, Proto::Keys::Divide,
        Proto::Keys::Left, Proto::Keys::Right, Proto::Keys::Up, Proto::Keys::Down,
        Proto::Keys::Numpad0, Proto::Keys::Numpad1, Proto::Keys::Numpad2, Proto::Keys::Numpad3, Proto::Keys::Numpad4,
        Proto::Keys::Numpad5, Proto::Keys::Numpad6, Proto::Keys::Numpad7, Proto::Keys::Numpad8, Proto::Keys::Numpad9,
        Proto::Keys::F1, Proto::Keys::F2, Proto::Keys::F3, Proto::Keys::F4, Proto::Keys::F5, Proto::Keys::F6,
        Proto::Keys::F7, Proto::Keys::F8, Proto::Keys::F9, Proto::Keys::F10, Proto::Keys::F11, Proto::Keys::F12,
        Proto::Keys::F13, Proto::Keys::F14, Proto::Keys::F15
    };

    class ValueRenderer {
        public:
            static ValueRenderer &getInstance()
            {
                static ValueRenderer instance;
                return instance;
            };

            void renderField(const std::string &name, ECS::Reflection::FieldType type,
                             ECS::Reflection::FieldValue value);

        private:
            std::unordered_map<ECS::Reflection::FieldType, std::function<void(
                const std::string &, ECS::Reflection::FieldValue)> > _renderingMap;

            ValueRenderer();

            static void renderFloat(const std::string &name, float *value);
            static void renderInt(const std::string &name, int *value);
            static void renderBool(const std::string &name, bool *value);
            static void renderVec2f(const std::string &name, Proto::Vec2f *value);
            static void renderString(const std::string &name, std::string *value);
            static void renderKeyMapLayout(const std::string &name,
                                           std::map<ECS::Entity::Moves, Proto::Key> *value);
            static void renderColor(const std::string &name, Proto::Color *value);
            
            static ECS::Entity::Moves _recordingMove;
            static bool _isRecording;
    };
}

#endif //PROTO_VALUERENDERER_HPP
