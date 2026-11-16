#include "core/Logger/Logger.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

void run_command_registry_tests();
void run_logger_tests();
void run_frame_timer_tests();
void run_event_manager_tests();
void run_gui_context_tests();
void run_panel_manager_tests();
void run_action_registry_tests();
void run_command_palette_tests();
void run_menu_bar_tests();
void run_toolbar_tests();
void run_layout_tests();
void run_workspace_tests();
void run_play_controller_tests();
void run_demo_scene_tests();
void run_demo_simulation_tests();
void run_ecs_scene_tests();
void run_ecs_simulation_tests();
void run_widget_tests();
void run_dialog_tests();
void run_notification_tests();
void run_theme_tests();
void run_hierarchy_panel_tests();
void run_inspector_panel_tests();
void run_console_panel_tests();
void run_shortcuts_panel_tests();
void run_panels_tests();
void run_editor_tests();

int main()
{
    // The editor writes imgui.ini, shortcuts.ini and its log in the working
    // directory: the tests run in an empty one, removed at the end.
    namespace fs = std::filesystem;
    const fs::path previous = fs::current_path();
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path directory = fs::temp_directory_path() / ("metrov_editor_tests_" + std::to_string(stamp));
    fs::create_directories(directory);
    fs::current_path(directory);
    // Errors are part of the tests: the terminal only gets what fails.
    Logger::instance().setTerminalLevel(LogLevel::Error);

    run_command_registry_tests();
    run_logger_tests();
    run_frame_timer_tests();
    run_event_manager_tests();
    run_gui_context_tests();
    run_panel_manager_tests();
    run_action_registry_tests();
    run_command_palette_tests();
    run_menu_bar_tests();
    run_toolbar_tests();
    run_layout_tests();
    run_workspace_tests();
    run_play_controller_tests();
    run_demo_scene_tests();
    run_demo_simulation_tests();
    run_ecs_scene_tests();
    run_ecs_simulation_tests();
    run_widget_tests();
    run_dialog_tests();
    run_notification_tests();
    run_theme_tests();
    run_hierarchy_panel_tests();
    run_inspector_panel_tests();
    run_console_panel_tests();
    run_shortcuts_panel_tests();
    run_panels_tests();
    run_editor_tests();

    fs::current_path(previous);
    std::error_code ignored;
    fs::remove_all(directory, ignored);
}
