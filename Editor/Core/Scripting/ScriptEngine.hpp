/*
    Created on: 3/9/26
    Filename: ScriptEngine.hpp
    Description: ScriptEngine stub — Phase 1 skeleton.
                 Tracks which script path is attached to each entity.
                 No Mono / C# execution yet.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_SCRIPTENGINE_HPP
#define PROTO_SCRIPTENGINE_HPP

#include <string>
#include <unordered_map>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include "Entity.hpp"
#include "Registry.hpp"
#include "SparseSet.hpp"
#include "FileWatcher.hpp"

namespace Editor::Scripting {

    struct MonoScriptInstance {
        MonoObject *object   = nullptr;
        MonoMethod *onStart  = nullptr;
        MonoMethod *onUpdate = nullptr;
        MonoMethod *onDestroy= nullptr;
        bool        started  = false;
    };

    class ScriptEngine {
    public:
        static ScriptEngine &getInstance();

        ScriptEngine(const ScriptEngine &) = delete;
        ScriptEngine &operator=(const ScriptEngine &) = delete;

        // Phase 5 — init/shutdown Mono runtime
        void init();
        void shutdown();

        // Load a compiled .dll containing user scripts
        void loadAssembly(const std::string &dllPath);

        static void attachScript(ECS::Entity::entity const &entity,
                                 const std::string &scriptPath,
                                 const std::string &className,
                                 const std::string &namespaceName,
                                 ECS::registry &reg);

        void detachScript(ECS::Entity::entity const &entity,
                          ECS::registry &reg);

        static bool hasScript(ECS::Entity::entity const &entity,
                              ECS::registry &reg);

        // Instantiates C# classes for all ScriptComponents that don't have one yet
        void bindAll(ECS::registry &reg);

        // Calls OnUpdate(dt) on all bound C# instances
        void tickAll(ECS::registry &reg, float dt);

        // Reloads the assembly — called by FileWatcher on Modified
        void reloadAssembly(const std::string &dllPath, ECS::registry &reg);

        // Called every frame — ticks the file watcher
        void checkHotReload(ECS::registry &reg);

        // Clears all live C# instances — call after a play session ends
        void resetInstances();

        // Set / clear the registry pointer used by native P/Invoke bindings
        void setActiveRegistry(ECS::registry *reg) { _activeRegistry = reg; }

        // Must be called at the start/end of any non-main thread that invokes Mono APIs
        void attachThread();
        void detachThread();

        // Recompile all .cs files in scriptsDir → outputDll using mcs
        static bool recompileScripts(const std::string &scriptsDir,
                                     const std::string &outputDll);

    private:
        ScriptEngine()  = default;
        ~ScriptEngine() = default;

        void registerNatives();
        MonoScriptInstance createInstance(const std::string &namespaceName,
                                          const std::string &className);

        // P/Invoke native bindings — called from C# via [MethodImpl(MethodImplOptions.InternalCall)]
        static void   Binding_GetPosition(std::size_t entityId, float *x, float *y);
        static void   Binding_SetPosition(std::size_t entityId, float x, float y);
        static void   Binding_GetVelocity(std::size_t entityId, float *vx, float *vy);
        static void   Binding_SetVelocity(std::size_t entityId, float vx, float vy);


        MonoDomain   *_domain      = nullptr;
        MonoDomain   *_childDomain = nullptr;   // child domain used for hot-reload
        MonoAssembly *_assembly    = nullptr;
        MonoImage    *_image       = nullptr;

        // Active registry — set at the start of each play session, cleared at end
        ECS::registry *_activeRegistry = nullptr;

        // entity id → C# instance
        std::unordered_map<std::size_t, MonoScriptInstance> _instances;

        FileWatcher _watcher;
    };

} // Editor::Scripting

#endif //PROTO_SCRIPTENGINE_HPP

