#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Scenes/Scene.hpp"
#include "Engine/Scenes/SceneRegistry.hpp"
#include "Engine/Serialization/Serialization.hpp"

/**
 * @brief A scene built from a scene file: its entities and its systems are
 *        data (made in the editor), not C++ code.
 *
 * The file describes what the scene holds: its entities, and the names of
 * its systems (the "systems" block), in order. The code defines which systems
 * exist (ECS::SystemCatalog, E_SYSTEM).
 *
 * Built in two steps:
 * 1. fromText() / fromFile() load the entities. A text that cannot be loaded
 *    gives no scene (nullptr), so SceneManager::push(id) pushes nothing.
 * 2. onEnter() creates the systems of the file, in order, then runs the
 *    optional setup of the code. It is called by the SceneManager after the
 *    Engine injected its services, so a system's init() sees both the
 *    entities and the services.
 *
 * Loading is tolerant, like the serialization: an unknown system, or one
 * that cannot be created, is a warning and the scene goes on without it.
 * An exception from a system (constructor or init()) or from the setup is
 * an error, and never leaves onEnter(): the scene is already on the stack
 * when it is called, so push() must not throw. The other systems are still
 * created.
 * Every error and warning is printed on std::cerr, as
 * "[scene <name>] error|warning: ...", and kept in loadReport().
 *
 * A subclass that overrides onEnter() must call DataScene::onEnter().
 */
class DataScene : public Scene {
    public:
        /** @brief Code run after the file's systems are created (adds systems of its own...). */
        using Setup = std::function<void(DataScene&)>;

        /**
         * @brief Told, by code, how a load went: the scene's id (its name)
         *        and its report (errors and warnings).
         *
         * Logs are for a human; this is for the code that pushed the scene
         * and must act on the result: mark a scene as broken in the editor,
         * fail a test, go back to a menu...
         */
        using ReportHandler = std::function<void(const std::string& id, const serial::Report& report)>;

        /**
         * @brief Builds a scene from the text of a scene file.
         *
         * The editor's Play uses it on the text of an open scene
         * (serial::serialize), so that the scene file is not written.
         * @param report Also receives the errors and warnings of the load.
         * @return The scene, or nullptr when the text cannot be loaded.
         */
        static std::unique_ptr<DataScene> fromText(std::string_view text, std::string name = "DataScene",
                                                   serial::Report* report = nullptr);

        /**
         * @brief Builds a scene from a scene file (serial::loadFile).
         * @param name Name of the scene; empty: the file name without extension.
         * @return The scene, or nullptr when the file cannot be read or loaded.
         */
        static std::unique_ptr<DataScene> fromFile(const std::filesystem::path& file, std::string name = {},
                                                   serial::Report* report = nullptr);

        /**
         * @brief Creates the file's systems, then runs the setup. Done once,
         *        even if called again. Never throws for a system or the
         *        setup: their exceptions are errors in loadReport().
         */
        void onEnter() override;

        /** @brief Code to run in onEnter(), after the file's systems. */
        void setSetup(Setup setup) { _setup = std::move(setup); }

        /**
         * @brief Called once, at the end of the first onEnter(), with the
         *        whole loadReport(): the load, the systems and the setup.
         *
         * It is called even when everything went well (report.ok()). An
         * exception it throws does not leave onEnter(): it is printed and
         * added to loadReport(), and the handler is not called again.
         */
        void setReportHandler(ReportHandler handler) { _onReport = std::move(handler); }

        /**
         * @brief Replaces the systems of the data: what save() writes, and
         *        what the next load creates. The editor adds, removes and
         *        reorders a scene's systems with it.
         *
         * Called before onEnter(), it changes which systems onEnter() creates.
         * Called after, it does not touch the systems already running:
         * onEnter() created them once. An empty name is skipped with a
         * warning (like at load time);
         * unknown names are kept, as save() keeps them.
         *
         * Its warnings are printed on std::cerr and added to `report`, like
         * save()'s; they do not go to loadReport(), which describes the load.
         * @param report Receives the warnings (may be null).
         */
        void setSystems(std::vector<std::string> systems, serial::Report* report = nullptr);

        /**
         * @brief Writes the scene to a scene file: its entities, and the
         *        systems of its info() (the "systems" block).
         *
         * Only the systems that came from the data are written, not the ones
         * the code added (setup, onEnter of a subclass): those are added again
         * by the code on every load, so writing them would create them twice.
         * Entities without a Uuid get one (SaveOptions::assignMissingUuids),
         * so that saving twice writes the same ids.
         * Errors and warnings are printed on std::cerr, like the load's.
         * @param report Also receives the errors and warnings of the save.
         * @return false when the file could not be written (it is then untouched).
         */
        bool save(const std::filesystem::path& file, serial::Report* report = nullptr);

        /** @brief What the file holds besides its entities (the names of its systems). */
        const serial::SceneInfo& info() const { return _info; }

        /**
         * @brief Every error and warning of the scene's load: the file, the
         *        creation of its systems, the setup and the report handler.
         *
         * What happens later to the scene (setSystems(), save()) is not
         * added: those report to the Report* they are given.
         */
        const serial::Report& loadReport() const { return _report; }

    private:
        // Loads a world and its SceneInfo (serial::load or serial::loadFile).
        using Loader = std::function<bool(ECS::Registry&, serial::Report*, serial::SceneInfo*)>;

        explicit DataScene(std::string name) : Scene(std::move(name)) {}

        // What fromText and fromFile share: creates the scene, loads it,
        // publishes the report; nullptr when the load fails.
        static std::unique_ptr<DataScene> build(std::string name, const Loader& load, serial::Report* report);

        void createSystems();
        void warn(std::string message);    // records and prints a warning
        void error(std::string message);   // records and prints an error (nothing stops)

        void reportResult();               // calls the report handler, if any (never throws)

        serial::SceneInfo _info;
        serial::Report _report;
        Setup _setup;
        ReportHandler _onReport;
        bool _systemsCreated = false;
};

/**
 * @brief Registers under `id` a factory that builds a DataScene from `file`.
 *
 * The file is read on every push(id): popping and pushing the scene again
 * starts it from the file. A file that cannot be loaded makes push(id)
 * return false; its errors are printed on std::cerr.
 * @param setup Run in onEnter(), after the file's systems (may be empty).
 * @param onReport Called once per push(id), with the id and the report: at
 *        the end of onEnter() when the scene was created (load, systems and
 *        setup together), or right away, with the load's errors, when the
 *        file cannot be loaded (push(id) then returns false). May be empty.
 * @return false (nothing registered) when the id is empty or already taken.
 */
bool addDataScene(SceneRegistry& registry, const std::string& id, std::filesystem::path file,
                  DataScene::Setup setup = {}, DataScene::ReportHandler onReport = {});
