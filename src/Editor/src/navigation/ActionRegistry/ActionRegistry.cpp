/*
    Created on: 30/03/2026
    Filename: ActionRegistry.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ActionRegistry.hpp"
#include "core/Logger/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace {

// ImGui names punctuation keys ("GraveAccent"); shortcuts show the symbol.
const std::pair<ImGuiKey, const char *> KeySymbols[] = {{ImGuiKey_GraveAccent, "`"}, {ImGuiKey_Apostrophe, "'"},
    {ImGuiKey_Comma, ","}, {ImGuiKey_Minus, "-"}, {ImGuiKey_Period, "."}, {ImGuiKey_Slash, "/"},
    {ImGuiKey_Semicolon, ";"}, {ImGuiKey_Equal, "="}, {ImGuiKey_LeftBracket, "["}, {ImGuiKey_Backslash, "\\"},
    {ImGuiKey_RightBracket, "]"}, {ImGuiKey_Delete, "Del"}};

std::string toLower(std::string text)
{
    std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

std::string trim(const std::string &text)
{
    const std::size_t begin = text.find_first_not_of(" \t\r");
    if (begin == std::string::npos)
        return "";
    const std::size_t end = text.find_last_not_of(" \t\r");
    return text.substr(begin, end - begin + 1);
}

// std::ranges::contains is C++23; the project is C++20.
template <typename Range, typename Value>
bool contains(const Range &range, const Value &value)
{
    return std::ranges::find(range, value) != std::ranges::end(range);
}

ImGuiKey keyOf(ImGuiKeyChord chord)
{
    return static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_);
}

// Keyboard keys that can end a shortcut (no mouse, gamepad or modifier key).
bool isShortcutKey(ImGuiKey key)
{
    if (key < ImGuiKey_NamedKey_BEGIN || key > ImGuiKey_Oem102)
        return false;
    return !(key >= ImGuiKey_LeftCtrl && key <= ImGuiKey_RightSuper);
}

} // namespace

Action &ActionRegistry::add(Action action)
{
    if (action.id.empty())
        throw std::invalid_argument("Action id must not be empty");
    if (has(action.id))
        throw std::invalid_argument("Action \"" + action.id + "\" is already registered");
    if (action.shortcut != ImGuiKey_None) {
        if (const Action *other = findByShortcut(action.shortcut))
            Logger::instance().warning("Shortcuts", shortcutText(action.shortcut) + " is used by both \"" + other->id
                    + "\" and \"" + action.id + "\"; only the first one will run");
    }
    _actions.push_back(std::make_unique<Action>(std::move(action)));
    const Action &added = *_actions.back();
    _shortcuts[added.id] = defaultShortcuts(added);
    return *_actions.back();
}

Action *ActionRegistry::get(const std::string &id) const
{
    const auto it = std::find_if(
        _actions.begin(), _actions.end(), [&id](const auto &action) { return action->id == id; });
    return it != _actions.end() ? it->get() : nullptr;
}

bool ActionRegistry::execute(const std::string &id) const
{
    const Action *action = get(id);
    if (!action || !action->enabled() || !action->callback)
        return false;
    action->callback();
    return true;
}

void ActionRegistry::processShortcuts() const
{
    // While typing in a text field, shortcuts without Ctrl/Alt/Super (e.g. a
    // plain letter or Delete) belong to the field, not to the editor.
    // Function keys (F1-F12) never type text, so they always work.
    const bool typing = ImGui::GetIO().WantTextInput;
    const ImGuiKeyChord commandMods = ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiMod_Super;

    for (const auto &action : _actions) {
        for (const ImGuiKeyChord chord : shortcuts(action->id)) {
            const ImGuiKey key = keyOf(chord);
            const bool functionKey = key >= ImGuiKey_F1 && key <= ImGuiKey_F12;
            if (typing && !(chord & commandMods) && !functionKey)
                continue;
            if (ImGui::Shortcut(chord, ImGuiInputFlags_RouteGlobal) && action->enabled() && action->callback) {
                action->callback();
                break;
            }
        }
    }
}

// ---- Shortcuts

std::vector<ImGuiKeyChord> ActionRegistry::defaultShortcuts(const Action &action) const
{
    return action.shortcut != ImGuiKey_None ? std::vector<ImGuiKeyChord>{action.shortcut} : std::vector<ImGuiKeyChord>{};
}

void ActionRegistry::notifyChanged() const
{
    if (_onShortcutsChanged)
        _onShortcutsChanged();
}

const std::vector<ImGuiKeyChord> &ActionRegistry::shortcuts(const std::string &id) const
{
    static const std::vector<ImGuiKeyChord> none;
    const auto it = _shortcuts.find(id);
    return it != _shortcuts.end() ? it->second : none;
}

bool ActionRegistry::addShortcut(const std::string &id, ImGuiKeyChord chord)
{
    const auto it = _shortcuts.find(id);
    if (it == _shortcuts.end() || !isShortcutKey(keyOf(chord)) || contains(it->second, chord))
        return false;
    it->second.push_back(chord);
    notifyChanged();
    return true;
}

bool ActionRegistry::removeShortcut(const std::string &id, ImGuiKeyChord chord)
{
    const auto it = _shortcuts.find(id);
    if (it == _shortcuts.end() || std::erase(it->second, chord) == 0)
        return false;
    notifyChanged();
    return true;
}

void ActionRegistry::resetShortcuts(const std::string &id)
{
    const Action *action = get(id);
    if (!action)
        return;
    _shortcuts[id] = defaultShortcuts(*action);
    notifyChanged();
}

void ActionRegistry::resetAllShortcuts()
{
    for (const auto &action : _actions)
        _shortcuts[action->id] = defaultShortcuts(*action);
    notifyChanged();
}

bool ActionRegistry::hasDefaultShortcuts(const std::string &id) const
{
    const Action *action = get(id);
    return !action || shortcuts(id) == defaultShortcuts(*action);
}

const Action *ActionRegistry::findByShortcut(ImGuiKeyChord chord) const
{
    for (const auto &action : _actions)
        if (contains(shortcuts(action->id), chord))
            return action.get();
    return nullptr;
}

bool ActionRegistry::saveShortcuts(const std::string &path) const
{
    std::ofstream file(path, std::ios::trunc);
    if (!file)
        return false;
    file << "# Editor keyboard shortcuts that differ from the defaults.\n"
            "# <action id>=<shortcut>[,<shortcut>...]   (empty: no shortcut)\n";
    for (const auto &action : _actions) {
        if (hasDefaultShortcuts(action->id))
            continue;
        file << action->id << "=";
        const std::vector<ImGuiKeyChord> &chords = shortcuts(action->id);
        for (std::size_t i = 0; i < chords.size(); i++)
            file << (i > 0 ? "," : "") << shortcutText(chords[i]);
        file << "\n";
    }
    return static_cast<bool>(file);
}

void ActionRegistry::loadShortcuts(const std::string &path)
{
    std::ifstream file(path);
    if (!file)
        return;
    Logger &logger = Logger::instance();
    std::string line;
    int lineNumber = 0;
    int applied = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        const std::size_t equal = line.find('=');
        const std::string where = path + ":" + std::to_string(lineNumber);
        if (equal == std::string::npos) {
            logger.warning("Shortcuts", where + ": expected <action id>=<shortcuts>");
            continue;
        }
        const std::string id = trim(line.substr(0, equal));
        if (!has(id)) {
            logger.warning("Shortcuts", where + ": unknown action \"" + id + "\"");
            continue;
        }
        std::vector<ImGuiKeyChord> chords;
        bool valid = true;
        const std::string list = line.substr(equal + 1);
        std::size_t start = 0;
        while (start <= list.size()) {
            const std::size_t comma = list.find(',', start);
            // A lone "," key is written "Ctrl+," : a comma right after "+" is part of the key.
            std::size_t end = comma;
            if (comma != std::string::npos && comma > 0 && list[comma - 1] == '+')
                end = list.find(',', comma + 1);
            const std::string text = trim(list.substr(start, end == std::string::npos ? std::string::npos : end - start));
            if (!text.empty()) {
                if (const std::optional<ImGuiKeyChord> chord = parseShortcut(text))
                    chords.push_back(*chord);
                else {
                    logger.warning("Shortcuts", where + ": invalid shortcut \"" + text + "\", line ignored");
                    valid = false;
                }
            }
            if (end == std::string::npos)
                break;
            start = end + 1;
        }
        // A typo must not silently remove the command's shortcuts.
        if (!valid)
            continue;
        _shortcuts[id] = chords;
        applied++;
    }
    if (applied > 0)
        logger.info("Shortcuts", "Loaded " + std::to_string(applied) + " custom shortcut(s) from " + path);
}

std::string ActionRegistry::shortcutLabel(const Action &action) const
{
    const std::vector<ImGuiKeyChord> &chords = shortcuts(action.id);
    return !chords.empty() ? shortcutText(chords.front()) : action.shortcutHint;
}

std::string ActionRegistry::shortcutText(ImGuiKeyChord chord)
{
    if (chord == ImGuiKey_None)
        return "";
    std::string text;
    if (chord & ImGuiMod_Ctrl)
        text += "Ctrl+";
    if (chord & ImGuiMod_Shift)
        text += "Shift+";
    if (chord & ImGuiMod_Alt)
        text += "Alt+";
    if (chord & ImGuiMod_Super)
        text += "Super+";
    const ImGuiKey key = keyOf(chord);
    const auto symbol = std::ranges::find(KeySymbols, key, &std::pair<ImGuiKey, const char *>::first);
    text += symbol != std::end(KeySymbols) ? symbol->second : ImGui::GetKeyName(key);
    return text;
}

std::optional<ImGuiKeyChord> ActionRegistry::parseShortcut(const std::string &text)
{
    // Modifiers are the parts before the last "+" ("Ctrl+=" or "Ctrl++" style
    // keys: the key is whatever follows the last modifier).
    ImGuiKeyChord mods = 0;
    std::string rest = trim(text);
    while (true) {
        const std::size_t plus = rest.find('+');
        if (plus == std::string::npos || plus == rest.size() - 1)
            break;
        const std::string mod = toLower(trim(rest.substr(0, plus)));
        if (mod == "ctrl" || mod == "control")
            mods |= ImGuiMod_Ctrl;
        else if (mod == "shift")
            mods |= ImGuiMod_Shift;
        else if (mod == "alt")
            mods |= ImGuiMod_Alt;
        else if (mod == "super" || mod == "cmd" || mod == "win")
            mods |= ImGuiMod_Super;
        else
            break; // Not a modifier: the rest is the key
        rest = trim(rest.substr(plus + 1));
    }
    if (rest.empty())
        return std::nullopt;

    const std::string name = toLower(rest);
    for (const auto &[key, symbol] : KeySymbols)
        if (name == toLower(symbol))
            return mods | key;
    for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++)
        if (isShortcutKey(static_cast<ImGuiKey>(key)) && name == toLower(ImGui::GetKeyName(static_cast<ImGuiKey>(key))))
            return mods | key;
    return std::nullopt;
}

std::optional<std::string> ActionRegistry::reservedReason(ImGuiKeyChord chord)
{
    const ImGuiKey key = keyOf(chord);
    const ImGuiKeyChord mods = chord & ImGuiMod_Mask_;
    if (!isShortcutKey(key))
        return "only keyboard keys can be used";
    if (key == ImGuiKey_Escape)
        return "Escape cancels and closes popups";
    if (key == ImGuiKey_Tab && (mods == 0 || mods == ImGuiMod_Shift || (mods & ImGuiMod_Ctrl)))
        return "Tab moves between widgets and Ctrl+Tab between panels";
    if (mods == 0 || mods == ImGuiMod_Shift) {
        static const ImGuiKey navigation[] = {ImGuiKey_Enter, ImGuiKey_KeypadEnter, ImGuiKey_Space, ImGuiKey_Backspace,
            ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_UpArrow, ImGuiKey_DownArrow, ImGuiKey_Home, ImGuiKey_End,
            ImGuiKey_PageUp, ImGuiKey_PageDown};
        if (contains(navigation, key))
            return std::string(ImGui::GetKeyName(key)) + " is used to navigate and edit text";
    }
    return std::nullopt;
}

std::string ActionRegistry::categoryOf(const std::string &id)
{
    static const std::pair<const char *, const char *> prefixes[] = {{"view.panel.", "Panel"},
        {"view.theme.", "Theme"}, {"window.", "Window"}, {"file.", "File"}, {"edit.", "Edit"}, {"view.", "View"},
        {"help.", "Help"}};
    for (const auto &[prefix, category] : prefixes)
        if (id.starts_with(prefix))
            return category;
    return "Command";
}
