/*
    Created on: 3/9/26
    Filename: ScriptEngine.cpp
    Description: Full Phase 5 Mono implementation.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "ScriptEngine.hpp"
#include "Entity/Base/Script/ScriptComponent.hpp"
#include "Entity/Base/Position/Position.hpp"
#include "Entity/Base/Velocity/Velocity.hpp"
#include "ProjectContext.hpp"

#include <mono/metadata/mono-config.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/exception.h>
#include <iostream>
#include <cstdlib>
#include <sstream>

namespace Editor::Scripting {

    ScriptEngine &ScriptEngine::getInstance()
    {
        static ScriptEngine instance;
        return instance;
    }

    // -------------------------------------------------------------------------
    // init() — boots the Mono JIT domain and registers native C++ bindings
    // -------------------------------------------------------------------------
    void ScriptEngine::init()
    {
        mono_config_parse(nullptr);
        _domain = mono_jit_init("MetrovyScriptDomain");
        if (!_domain) {
            std::cerr << "[ScriptEngine] Failed to initialize Mono JIT domain\n";
            return;
        }
        registerNatives();
        std::cout << "[ScriptEngine] Mono runtime initialized\n";
    }

    void ScriptEngine::shutdown()
    {
        _instances.clear();
        if (_childDomain) {
            mono_domain_set(_domain, false);
            mono_domain_unload(_childDomain);
            _childDomain = nullptr;
        }
        _assembly = nullptr;
        _image    = nullptr;
        if (_domain) {
            mono_jit_cleanup(_domain);
            _domain = nullptr;
        }
    }

    // -------------------------------------------------------------------------
    // loadAssembly() — loads a compiled .dll into a fresh child domain
    // -------------------------------------------------------------------------
    void ScriptEngine::loadAssembly(const std::string &dllPath)
    {
        if (!_domain) {
            std::cerr << "[ScriptEngine] Cannot load assembly — Mono not initialized\n";
            return;
        }

        // Clean up any existing child domain
        if (_childDomain) {
            mono_domain_set(_domain, false);
            mono_domain_unload(_childDomain);
            _childDomain = nullptr;
            _assembly    = nullptr;
            _image       = nullptr;
        }

        _childDomain = mono_domain_create_appdomain(
            const_cast<char *>("MetrovyScripts"), nullptr);
        if (!_childDomain) {
            std::cerr << "[ScriptEngine] Failed to create script domain\n";
            return;
        }
        mono_domain_set(_childDomain, false);

        _assembly = mono_domain_assembly_open(_childDomain, dllPath.c_str());
        if (!_assembly) {
            std::cerr << "[ScriptEngine] Failed to load assembly: " << dllPath << "\n";
            return;
        }
        _image = mono_assembly_get_image(_assembly);
        std::cout << "[ScriptEngine] Assembly loaded: " << dllPath << "\n";
    }

    // -------------------------------------------------------------------------
    // attachScript() — stores ScriptComponent data on the entity (data only)
    // -------------------------------------------------------------------------
    void ScriptEngine::attachScript(ECS::Entity::entity const &entity,
                                    const std::string &scriptPath,
                                    const std::string &className,
                                    const std::string &namespaceName,
                                    ECS::registry &reg)
    {
        auto *arr = reg.get_if<ECS::Entity::ScriptComponent>();
        if (arr) {
            using idx_t = ECS::sparse_set<ECS::Entity::ScriptComponent>::size_type;
            auto idx = static_cast<idx_t>(static_cast<std::size_t>(entity));
            if (arr->has(idx)) {
                auto &comp = reg.get_component<ECS::Entity::ScriptComponent>(entity);
                comp.scriptPath    = scriptPath;
                comp.className     = className;
                comp.namespaceName = namespaceName;
                comp.active        = true;
                return;
            }
        }
        reg.add_component(entity, ECS::Entity::ScriptComponent{scriptPath, className, namespaceName});
    }

    void ScriptEngine::detachScript(ECS::Entity::entity const &entity, ECS::registry &reg)
    {
        reg.remove_component<ECS::Entity::ScriptComponent>(entity);
        _instances.erase(static_cast<std::size_t>(entity));
    }

    bool ScriptEngine::hasScript(ECS::Entity::entity const &entity, ECS::registry &reg)
    {
        auto *arr = reg.get_if<ECS::Entity::ScriptComponent>();
        if (!arr) return false;
        using idx_t = ECS::sparse_set<ECS::Entity::ScriptComponent>::size_type;
        auto idx = static_cast<idx_t>(static_cast<std::size_t>(entity));
        return arr->has(idx);
    }

    // -------------------------------------------------------------------------
    // createInstance() — instantiates a C# class from the loaded assembly
    // -------------------------------------------------------------------------
    MonoScriptInstance ScriptEngine::createInstance(const std::string &ns,
                                                     const std::string &className)
    {
        MonoScriptInstance inst;
        if (!_image) return inst;

        MonoClass *klass = mono_class_from_name(_image, ns.c_str(), className.c_str());
        if (!klass) {
            std::cerr << "[ScriptEngine] Class not found: " << ns << "." << className << "\n";
            return inst;
        }

        inst.object = mono_object_new(_childDomain ? _childDomain : _domain, klass);
        if (!inst.object) return inst;
        mono_runtime_object_init(inst.object);

        inst.onStart   = mono_class_get_method_from_name(klass, "OnStart",   0);
        inst.onUpdate  = mono_class_get_method_from_name(klass, "OnUpdate",  1);
        inst.onDestroy = mono_class_get_method_from_name(klass, "OnDestroy", 0);

        return inst;
    }

    // -------------------------------------------------------------------------
    // bindAll() — for every ScriptComponent without a live instance, create one
    // -------------------------------------------------------------------------
    void ScriptEngine::bindAll(ECS::registry &reg)
    {
        if (!_image) return;

        auto *arr = reg.get_if<ECS::Entity::ScriptComponent>();
        if (!arr) return;

        for (auto &entity : reg.get_all_entities()) {
            using idx_t = ECS::sparse_set<ECS::Entity::ScriptComponent>::size_type;
            auto idx = static_cast<idx_t>(static_cast<std::size_t>(entity));
            if (!arr->has(idx)) continue;

            auto &comp = arr->get(idx);
            if (!comp.active || comp.className.empty() || comp.scriptPath.empty()) continue;

            const std::size_t eid = static_cast<std::size_t>(entity);
            if (_instances.count(eid)) continue;   // already bound

            auto inst = createInstance(comp.namespaceName, comp.className);
            if (inst.object) {
                // Set EntityId property so the script knows which entity it belongs to
                MonoClass  *klass    = mono_object_get_class(inst.object);
                MonoProperty *propId = mono_class_get_property_from_name(klass, "EntityId");
                if (propId) {
                    MonoMethod *setter = mono_property_get_set_method(propId);
                    if (setter) {
                        std::uint64_t id = static_cast<std::uint64_t>(eid);
                        void *args[] = { &id };
                        MonoObject *exc = nullptr;
                        mono_runtime_invoke(setter, inst.object, args, &exc);
                        if (exc) mono_print_unhandled_exception(exc);
                    }
                }
                _instances[eid] = inst;
            }
        }
    }

    // -------------------------------------------------------------------------
    // tickAll() — calls OnStart (once) then OnUpdate(dt) on every live instance
    // -------------------------------------------------------------------------
    void ScriptEngine::tickAll(ECS::registry &reg, float dt)
    {
        if (!_image) return;

        _activeRegistry = &reg;
        bindAll(reg);

        for (auto &[eid, inst] : _instances) {
            if (!inst.object) continue;

            // OnStart — called exactly once
            if (!inst.started) {
                if (inst.onStart) {
                    MonoObject *exc = nullptr;
                    mono_runtime_invoke(inst.onStart, inst.object, nullptr, &exc);
                    if (exc) mono_print_unhandled_exception(exc);
                }
                inst.started = true;
            }

            // OnUpdate(float dt)
            if (inst.onUpdate) {
                void *args[] = { &dt };
                MonoObject *exc = nullptr;
                mono_runtime_invoke(inst.onUpdate, inst.object, args, &exc);
                if (exc) mono_print_unhandled_exception(exc);
            }
        }
    }

    // -------------------------------------------------------------------------
    // resetInstances() — clears live instances; call at end of each play session
    // -------------------------------------------------------------------------
    void ScriptEngine::resetInstances()
    {
        _instances.clear();
        _activeRegistry = nullptr;
    }

    // -------------------------------------------------------------------------
    // attachThread() / detachThread()
    // Must be called on any thread that will invoke Mono APIs (e.g. the game thread).
    // Mono's GC requires every native thread to be registered before use.
    // -------------------------------------------------------------------------
    void ScriptEngine::attachThread()
    {
        MonoDomain *active = _childDomain ? _childDomain : _domain;
        if (active)
            mono_thread_attach(active);
    }

    void ScriptEngine::detachThread()
    {
        MonoThread *t = mono_thread_current();
        if (t)
            mono_thread_detach(t);
    }

    // -------------------------------------------------------------------------
    // recompileScripts() — invokes mcs to rebuild all .cs files → Scripts.dll
    // Returns true on success.
    // -------------------------------------------------------------------------
    bool ScriptEngine::recompileScripts(const std::string &scriptsDir,
                                         const std::string &outputDll)
    {
        // Collect every .cs file in the scripts directory
        std::string sources;
        for (auto &entry : std::filesystem::directory_iterator(scriptsDir)) {
            if (entry.path().extension() == ".cs") {
                sources += " \"" + entry.path().string() + "\"";
            }
        }
        if (sources.empty()) {
            std::cerr << "[ScriptEngine] No .cs files found in: " << scriptsDir << "\n";
            return false;
        }

        const std::string cmd = "mcs -target:library -out:\"" + outputDll + "\"" + sources + " 2>&1";
        std::cout << "[ScriptEngine] Recompiling: " << cmd << "\n";

        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "[ScriptEngine] Failed to run mcs\n";
            return false;
        }
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe))
            std::cout << "[mcs] " << buf;
        const int ret = pclose(pipe);

        if (ret != 0) {
            std::cerr << "[ScriptEngine] Compilation failed (exit " << ret << ")\n";
            return false;
        }
        std::cout << "[ScriptEngine] Compilation succeeded → " << outputDll << "\n";
        return true;
    }

    // -------------------------------------------------------------------------
    // reloadAssembly() — hot-reload using a child domain so the old image is
    // properly discarded before loading the new one.
    // -------------------------------------------------------------------------
    void ScriptEngine::reloadAssembly(const std::string &dllPath, ECS::registry &reg)
    {
        std::cout << "[ScriptEngine] Hot-reloading assembly: " << dllPath << "\n";
        _instances.clear();

        // Unload the child domain if one exists, releasing the old image/assembly
        if (_childDomain) {
            mono_domain_set(_domain, false);   // switch back to root domain first
            mono_domain_unload(_childDomain);
            _childDomain = nullptr;
            _assembly    = nullptr;
            _image       = nullptr;
        }

        // Create a fresh child domain for the new assembly
        _childDomain = mono_domain_create_appdomain(
            const_cast<char *>("MetrovyScripts"), nullptr);
        if (!_childDomain) {
            std::cerr << "[ScriptEngine] Failed to create child domain\n";
            return;
        }
        mono_domain_set(_childDomain, false);

        _assembly = mono_domain_assembly_open(_childDomain, dllPath.c_str());
        if (!_assembly) {
            std::cerr << "[ScriptEngine] Failed to load assembly in child domain: " << dllPath << "\n";
            return;
        }
        _image = mono_assembly_get_image(_assembly);
        std::cout << "[ScriptEngine] Assembly reloaded in child domain: " << dllPath << "\n";

        bindAll(reg);
    }

    // -------------------------------------------------------------------------
    // checkHotReload() — watches every .cs in scripts/ for changes,
    //                     auto-recompiles with mcs, then reloads Scripts.dll.
    // -------------------------------------------------------------------------
    void ScriptEngine::checkHotReload(ECS::registry &reg)
    {
        // Resolve the scripts directory from the project root
        const std::filesystem::path scriptsDir =
            Editor::ProjectContext::getInstance().resolve("scripts");
        const std::string dllPath =
            (scriptsDir / "Scripts.dll").string();

        if (!std::filesystem::exists(scriptsDir)) return;

        // Walk every .cs file and register a watcher if not already tracked
        for (auto &entry : std::filesystem::directory_iterator(scriptsDir)) {
            if (entry.path().extension() != ".cs") continue;

            const std::string csPath = entry.path().string();
            if (_watcher.isWatching(csPath)) continue;

            _watcher.watch(csPath, [this, scriptsDir, dllPath, &reg]
                (const std::string &changedPath, FileStatus status) {
                if (status == FileStatus::Modified || status == FileStatus::Created) {
                    std::cout << "[ScriptEngine] Detected change: " << changedPath << "\n";
                    if (recompileScripts(scriptsDir.string(), dllPath))
                        reloadAssembly(dllPath, reg);
                }
            });
        }

        _watcher.tick();
    }

    // -------------------------------------------------------------------------
    // registerNatives() — expose C++ functions callable from C# via P/Invoke
    // -------------------------------------------------------------------------
    void ScriptEngine::registerNatives()
    {
        mono_add_internal_call("Metrovy.Native::GetPosition",
            reinterpret_cast<void*>(Binding_GetPosition));
        mono_add_internal_call("Metrovy.Native::SetPosition",
            reinterpret_cast<void*>(Binding_SetPosition));
        mono_add_internal_call("Metrovy.Native::GetVelocity",
            reinterpret_cast<void*>(Binding_GetVelocity));
        mono_add_internal_call("Metrovy.Native::SetVelocity",
            reinterpret_cast<void*>(Binding_SetVelocity));
    }

    // -------------------------------------------------------------------------
    // Native binding implementations — called from C# scripts
    // -------------------------------------------------------------------------
    void ScriptEngine::Binding_GetPosition(std::size_t entityId, float *x, float *y)
    {
        auto *reg = getInstance()._activeRegistry;
        if (!reg || !x || !y) return;
        auto *arr = reg->get_if<ECS::Entity::Position>();
        if (!arr || !arr->has(entityId)) return;
        auto &pos = arr->get(entityId);
        *x = pos._x;
        *y = pos._y;
    }

    void ScriptEngine::Binding_SetPosition(std::size_t entityId, float x, float y)
    {
        auto *reg = getInstance()._activeRegistry;
        if (!reg) return;
        auto *arr = reg->get_if<ECS::Entity::Position>();
        if (!arr || !arr->has(entityId)) return;
        auto &pos = arr->get(entityId);
        pos._x = x;
        pos._y = y;
    }

    void ScriptEngine::Binding_GetVelocity(std::size_t entityId, float *vx, float *vy)
    {
        auto *reg = getInstance()._activeRegistry;
        if (!reg || !vx || !vy) return;
        auto *arr = reg->get_if<ECS::Entity::Velocity>();
        if (!arr || !arr->has(entityId)) return;
        auto &vel = arr->get(entityId);
        *vx = vel._vx;
        *vy = vel._vy;
    }

    void ScriptEngine::Binding_SetVelocity(std::size_t entityId, float vx, float vy)
    {
        auto *reg = getInstance()._activeRegistry;
        if (!reg) return;
        auto *arr = reg->get_if<ECS::Entity::Velocity>();
        if (!arr || !arr->has(entityId)) return;
        auto &vel = arr->get(entityId);
        vel._vx = vx;
        vel._vy = vy;
    }

} // Editor::Scripting

