/*
    Created on: 3/10/26
    Filename: ProjectContext.hpp
    Description: Singleton that anchors all relative paths used in the editor.
                 Captures the project source root at startup so that script paths
                 like "scripts/Player.cs" always resolve relative to the project root.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_PROJECTCONTEXT_HPP
#define PROTO_PROJECTCONTEXT_HPP

#include <filesystem>
#include <string>

namespace Editor {

    class ProjectContext {
    public:
        static ProjectContext &getInstance();

        ProjectContext(const ProjectContext &) = delete;
        ProjectContext &operator=(const ProjectContext &) = delete;

        [[nodiscard]] const std::filesystem::path &root() const;
        void setRoot(const std::filesystem::path &path);
        [[nodiscard]] std::filesystem::path resolve(const std::string &path) const;

        [[nodiscard]] float screenWidth() const { return _screenW; }
        [[nodiscard]] float screenHeight() const { return _screenH; }
        void setScreenSize(float w, float h) { _screenW = w; _screenH = h; }

    private:
        ProjectContext();
        std::filesystem::path _root;
        float _screenW = 1920.0f;
        float _screenH = 1080.0f;
    };

} // Editor

#endif //PROTO_PROJECTCONTEXT_HPP

