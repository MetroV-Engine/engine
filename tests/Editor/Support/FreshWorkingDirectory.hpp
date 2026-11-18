#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>

// The editor saves its layout and shortcuts in the working directory: each
// run gets a fresh one, removed afterwards.
class FreshWorkingDirectory
{
public:
    FreshWorkingDirectory()
        : _previous(std::filesystem::current_path()),
          _path(std::filesystem::temp_directory_path()
                / ("metrov-editor-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
    {
        std::filesystem::create_directories(_path);
        std::filesystem::current_path(_path);
    }
    ~FreshWorkingDirectory()
    {
        std::filesystem::current_path(_previous);
        std::error_code ignored;
        std::filesystem::remove_all(_path, ignored);
    }
    FreshWorkingDirectory(const FreshWorkingDirectory &) = delete;
    FreshWorkingDirectory &operator=(const FreshWorkingDirectory &) = delete;

private:
    std::filesystem::path _previous;
    std::filesystem::path _path;
};
