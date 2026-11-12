#pragma once

// Runs the editor's UI without a screen, and drives it like a user would.
//
// A real GuiContext is created on SDL's "dummy" video driver with the
// "software" renderer, so every ImGui call of the editor really runs. Input
// is given to ImGui directly (io.AddKeyEvent, AddInputCharactersUTF8, mouse
// events): ImGui processes it on the next frame, exactly as input coming from
// SDL. A widget is "clicked" by its ImGui id (ActivateItemByID, the
// activation keyboard navigation uses): the ids are computed from the window
// and the labels, the way ImGui computes them (see itemId()).

#include "core/GuiContext/GuiContext.hpp"
#include "core/Logger/Logger.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace EditorTest {

// SDL_Quit (in ~GuiContext) clears every hint: they are set again before each
// GuiContext. "offscreen" would fail on macOS (it asks every window for EGL).
struct HeadlessHints
{
    HeadlessHints()
    {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    }
};

// Popup to open at the start of the next frame (see openPopup).
inline ImGuiID &pendingPopup()
{
    static ImGuiID id = 0;
    return id;
}

// Opens a popup (a context menu...) at the start of the next frame, before
// the widgets that own it are drawn: a popup not drawn in the frame it is
// opened would be closed.
inline void openPopup(ImGuiID id)
{
    pendingPopup() = id;
}

// Makes this frame cheap to render. SDL's software renderer fills every
// pixel of every triangle, which takes ~300 ms for the whole editor in a
// debug build: drawn at 1/20 of its size, it takes ~10 ms. Only the rendering
// changes: ImGui lays out and handles input in DisplaySize, which stays the
// same. Call during the frame, before ImGui::Render().
inline void renderSmall()
{
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(0.05f, 0.05f);
}

// A GuiContext whose frames the test runs one by one.
class Gui : private HeadlessHints, public GuiContext
{
public:
    // Without an ini file unless asked: a test does not depend on the layout
    // an earlier test saved.
    explicit Gui(bool iniFile = false) : GuiContext("Editor tests", 1600, 900)
    {
        if (!iniFile)
            ImGui::GetIO().IniFilename = nullptr;
    }

    // One frame: events, a new ImGui frame, the widgets, rendering.
    void frame(const std::function<void()> &ui = {})
    {
        processEvents();
        beginFrame();
        if (const ImGuiID popup = pendingPopup()) {
            pendingPopup() = 0;
            ImGui::OpenPopupEx(popup);
        }
        if (ui)
            ui();
        renderSmall();
        present();
    }

    void frames(int count, const std::function<void()> &ui = {})
    {
        for (int i = 0; i < count; i++)
            frame(ui);
    }
};

// ---- Input, processed by ImGui on the next frame

// Presses and releases a key with its modifiers (ImGuiMod_Ctrl | ImGuiKey_N).
// ImGui sees the press on the next frame and the release on the one after.
inline void press(ImGuiKeyChord chord)
{
    ImGuiIO &io = ImGui::GetIO();
    const ImGuiKey key = static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_);
    const ImGuiKey mods[] = {ImGuiMod_Ctrl, ImGuiMod_Shift, ImGuiMod_Alt, ImGuiMod_Super};
    for (const ImGuiKey mod : mods)
        if (chord & mod)
            io.AddKeyEvent(mod, true);
    io.AddKeyEvent(key, true);
    io.AddKeyEvent(key, false);
    for (const ImGuiKey mod : mods)
        if (chord & mod)
            io.AddKeyEvent(mod, false);
}

// Types text into the focused field (on the next frame).
inline void typeText(const std::string &text)
{
    ImGui::GetIO().AddInputCharactersUTF8(text.c_str());
}

inline void mouseMove(ImVec2 position)
{
    ImGui::GetIO().AddMousePosEvent(position.x, position.y);
}

inline void mouseButton(ImGuiMouseButton button, bool down)
{
    ImGui::GetIO().AddMouseButtonEvent(button, down);
}

// Moves the mouse there, then presses and releases a button (over three frames).
inline void click(ImVec2 position, ImGuiMouseButton button = ImGuiMouseButton_Left)
{
    mouseMove(position);
    mouseButton(button, true);
    mouseButton(button, false);
}

// ---- Finding windows and widgets

// The window named exactly name ("Console", "##Toolbar", "File###Menu_00"
// is found as "###Menu_00"), or the first whose name starts with name (a
// child window: "Hierarchy/##HierarchyTree_"). nullptr if none was drawn.
inline ImGuiWindow *window(const std::string &name)
{
    if (ImGuiWindow *exact = ImGui::FindWindowByName(name.c_str()))
        return exact;
    for (ImGuiWindow *candidate : GImGui->Windows)
        if (std::string(candidate->Name).starts_with(name))
            return candidate;
    return nullptr;
}

// The window of an open popup (BeginPopup / BeginPopupModal / context menus).
inline ImGuiWindow *popupWindow(ImGuiID popupId)
{
    char name[20];
    ImFormatString(name, IM_COUNTOF(name), "##Popup_%08x", popupId);
    return ImGui::FindWindowByName(name);
}

// The open menu at depth (0: a menu of the menu bar, 1: its submenu).
inline ImGuiWindow *menuWindow(int depth = 0)
{
    char name[16];
    ImFormatString(name, IM_COUNTOF(name), "###Menu_%02d", depth);
    return ImGui::FindWindowByName(name);
}

// PushID(pointer). A type of its own: a string literal would otherwise
// convert to const void * before std::string.
struct PointerId
{
    const void *pointer;
};

// One level of ImGui's ID stack: a label or PushID(string), PushID(int), or
// PushID(pointer).
using IdPart = std::variant<std::string, int, PointerId>;

// Id of a widget: the window's id, then each PushID scope and the label, as
// ImGui hashes them (BeginTable, BeginChild and BeginMenuBar push scopes too:
// give their str_id; a menu bar is "##MenuBar").
inline ImGuiID itemId(ImGuiID seed, const std::vector<IdPart> &path)
{
    for (const IdPart &part : path) {
        if (const std::string *text = std::get_if<std::string>(&part))
            seed = ImHashStr(text->c_str(), 0, seed);
        else if (const int *number = std::get_if<int>(&part))
            seed = ImHashData(number, sizeof(*number), seed);
        else {
            const void *pointer = std::get<PointerId>(part).pointer;
            seed = ImHashData(&pointer, sizeof(pointer), seed);
        }
    }
    return seed;
}

inline ImGuiID itemId(const ImGuiWindow *in, const std::vector<IdPart> &path)
{
    return in ? itemId(in->ID, path) : 0;
}

// Activates the widget on the next frame, as Enter on a focused widget does:
// a button is pressed, a checkbox toggled, a menu opened, a text field gets
// the keyboard. preferInput: a drag or slider becomes a text field instead.
inline void activate(ImGuiID id, bool preferInput = false)
{
    ImGui::ActivateItemByID(id);
    if (preferInput)
        GImGui->NavNextActivateFlags = ImGuiActivateFlags_PreferInput;
}

// ---- The log

// Remembers where the log was, to look only at what was logged since.
class LogWatch
{
public:
    LogWatch() { _start = lastId(); }

    // A message logged since, containing text (and of this level, if given).
    bool contains(const std::string &text) const { return count(text) > 0; }
    bool contains(LogLevel level, const std::string &text) const { return count(text, &level) > 0; }
    std::size_t count(LogLevel level, const std::string &text) const { return count(text, &level); }

    std::size_t count(const std::string &text, const LogLevel *level = nullptr) const
    {
        std::vector<LogEntry> entries;
        Logger::instance().copyEntries(entries);
        std::size_t found = 0;
        for (const LogEntry &entry : entries)
            if (entry.id > _start && entry.message.find(text) != std::string::npos && (!level || entry.level == *level))
                found += static_cast<std::size_t>(entry.count);
        return found;
    }

    void reset() { _start = lastId(); }

    // Everything logged since, one message per line (to debug a test).
    std::string text() const
    {
        std::vector<LogEntry> entries;
        Logger::instance().copyEntries(entries);
        std::string result;
        for (const LogEntry &entry : entries)
            if (entry.id > _start)
                result += "[" + entry.category + "] " + entry.message + "\n";
        return result;
    }

private:
    std::uint64_t _start = 0;

    static std::uint64_t lastId()
    {
        std::vector<LogEntry> entries;
        Logger::instance().copyEntries(entries);
        return entries.empty() ? 0 : entries.back().id;
    }
};

// ---- Scripts: what a user does, frame after frame

// Steps run one per frame, from inside the frame (e.g. from the Viewport's
// renderer while Editor::run() loops). Input given at a step reaches ImGui
// on the next frame.
class Script
{
public:
    using Step = std::function<void()>;

    Script &then(Step step)
    {
        _steps.push_back(std::move(step));
        return *this;
    }
    Script &wait(int frames = 1)
    {
        for (int i = 0; i < frames; i++)
            _steps.emplace_back();
        return *this;
    }
    // A shortcut, then a few frames for its effect.
    Script &keys(ImGuiKeyChord chord, int frames = 3)
    {
        then([chord] { press(chord); });
        return wait(frames);
    }
    // Text typed into the focused field, then Enter: text, Enter down and
    // Enter up reach ImGui on three frames.
    Script &enter(const std::string &text, int frames = 4)
    {
        then([text] {
            typeText(text);
            press(ImGuiKey_Enter);
        });
        return wait(frames);
    }

    // Runs the next step; false when there is none left.
    bool step()
    {
        if (_steps.empty())
            return false;
        const Step current = std::move(_steps.front());
        _steps.pop_front();
        if (current)
            current();
        return true;
    }
    bool done() const { return _steps.empty(); }

private:
    std::deque<Step> _steps;
};

} // namespace EditorTest
