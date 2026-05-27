/*
    Created on: 3/10/26
    Filename: ProjectContext.cpp
    Description: Singleton implementation — resolves the project source root
                 at compile time using __FILE__ so paths are always relative
                 to MetroV-prototype/, not the binary's working directory.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ProjectContext.hpp"

namespace Editor {

    // __FILE__ expands to the absolute path of this .cpp at compile time:
    //   /home/.../MetroV-prototype/Editor/Core/ProjectContext.cpp
    // Walking up 2 parent directories gives the project root:
    //   /home/.../MetroV-prototype/
    static std::filesystem::path deriveProjectRoot()
    {
        std::filesystem::path thisFile(__FILE__);          // .../Editor/Core/ProjectContext.cpp
        return thisFile.parent_path()                      // .../Editor/Core
                       .parent_path()                      // .../Editor
                       .parent_path()                      // .../MetroV-prototype
                       .lexically_normal();
    }

    ProjectContext::ProjectContext()
        : _root(deriveProjectRoot())
    {}

    ProjectContext &ProjectContext::getInstance()
    {
        static ProjectContext instance;
        return instance;
    }

    const std::filesystem::path &ProjectContext::root() const
    {
        return _root;
    }

    void ProjectContext::setRoot(const std::filesystem::path &path)
    {
        _root = path;
    }

    std::filesystem::path ProjectContext::resolve(const std::string &path) const
    {
        if (path.empty()) return {};
        std::filesystem::path p(path);
        if (p.is_absolute()) return p;
        return (_root / p).lexically_normal();
    }

} // Editor

