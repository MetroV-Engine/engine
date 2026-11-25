#include "Engine/Scenes/DataScene.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <utility>

#include "ECS/Core/System/SystemCatalog.hpp"

namespace {
    // The one place the messages of a scene load are printed. Through
    // std::cerr until the engine has its logger: plug it in here.
    void print(const std::string& scene, const char* kind, const std::string& message) {
        std::cerr << "[scene " << scene << "] " << kind << ": " << message << '\n';
    }

    // Prints every message of report, then appends them to `to` (if any).
    void publish(const std::string& scene, const serial::Report& report, serial::Report* to) {
        for (const std::string& error : report.errors)
            print(scene, "error", error);
        for (const std::string& warning : report.warnings)
            print(scene, "warning", warning);
        if (to) {
            to->errors.insert(to->errors.end(), report.errors.begin(), report.errors.end());
            to->warnings.insert(to->warnings.end(), report.warnings.begin(), report.warnings.end());
        }
    }
}

std::unique_ptr<DataScene> DataScene::build(std::string name, const Loader& load, serial::Report* report)
{
    std::unique_ptr<DataScene> scene(new DataScene(std::move(name)));
    serial::Report loaded;
    const bool ok = load(scene->registry(), &loaded, &scene->_info);
    publish(scene->name(), loaded, report);
    if (!ok)
        return nullptr;
    scene->_report = std::move(loaded);
    return scene;
}

std::unique_ptr<DataScene> DataScene::fromText(std::string_view text, std::string name, serial::Report* report)
{
    return build(std::move(name), [text](ECS::Registry& world, serial::Report* rep, serial::SceneInfo* info) {
        return serial::load(world, text, rep, info);
    }, report);
}

std::unique_ptr<DataScene> DataScene::fromFile(const std::filesystem::path& file, std::string name,
                                               serial::Report* report)
{
    if (name.empty())
        name = file.stem().string();
    return build(std::move(name), [&file](ECS::Registry& world, serial::Report* rep, serial::SceneInfo* info) {
        return serial::loadFile(world, file, rep, info);
    }, report);
}

bool DataScene::save(const std::filesystem::path& file, serial::Report* report)
{
    serial::SaveOptions options;
    options.assignMissingUuids = true;
    serial::Report saved;
    const bool ok = serial::saveFile(registry(), file, &saved, options, &_info);
    publish(name(), saved, report);
    return ok;
}

void DataScene::onEnter()
{
    if (_systemsCreated)
        return;
    _systemsCreated = true;
    createSystems();
    // Same tolerance as the file's systems: the scene is already on the stack,
    // so an exception here must not leave push().
    if (_setup) {
        try {
            _setup(*this);
        } catch (const std::exception& thrown) {
            error(std::string("setup failed: ") + thrown.what());
        } catch (...) {
            error("setup failed: unknown error");
        }
    }
    reportResult();
}

void DataScene::reportResult()
{
    if (!_onReport)
        return;
    try {
        _onReport(name(), _report);
    } catch (const std::exception& thrown) {
        error(std::string("report handler failed: ") + thrown.what());
    } catch (...) {
        error("report handler failed: unknown error");
    }
}

void DataScene::setSystems(std::vector<std::string> systems, serial::Report* report)
{
    serial::Report edited;   // not loadReport(): this is not part of the load
    std::vector<std::string> kept;
    kept.reserve(systems.size());
    for (std::size_t i = 0; i < systems.size(); ++i) {
        if (systems[i].empty()) {
            edited.warnings.push_back("setSystems: systems[" + std::to_string(i) +
                                      "]: a system name must be a non-empty string, skipped");
            continue;
        }
        kept.push_back(std::move(systems[i]));
    }
    _info.systems = std::move(kept);
    publish(name(), edited, report);
}

void DataScene::createSystems()
{
    const ECS::SystemCatalog& catalog = ECS::SystemCatalog::instance();
    for (const std::string& systemName : _info.systems) {
        if (!catalog.has(systemName)) {
            warn("unknown system '" + systemName + "', skipped");
            continue;
        }
        try {
            if (!catalog.createInto(systemName, systems(), registry()))
                warn("system '" + systemName + "' cannot be created (see the system catalog's errors), skipped");
        } catch (const std::exception& thrown) {
            error("system '" + systemName + "' failed to start: " + thrown.what());
        } catch (...) {
            error("system '" + systemName + "' failed to start: unknown error");
        }
    }
}

void DataScene::warn(std::string message)
{
    print(name(), "warning", message);
    _report.warnings.push_back(std::move(message));
}

void DataScene::error(std::string message)
{
    print(name(), "error", message);
    _report.errors.push_back(std::move(message));
}

bool addDataScene(SceneRegistry& registry, const std::string& id, std::filesystem::path file, DataScene::Setup setup,
                  DataScene::ReportHandler onReport)
{
    return registry.add(id, [id, file = std::move(file), setup = std::move(setup),
                             onReport = std::move(onReport)]() -> std::unique_ptr<Scene> {
        serial::Report loaded;
        std::unique_ptr<DataScene> scene = DataScene::fromFile(file, id, &loaded);
        if (!scene) {
            // No scene, so no onEnter(): tell the handler now. push(id) returns false.
            if (onReport) {
                try {
                    onReport(id, loaded);
                } catch (const std::exception& thrown) {
                    print(id, "error", std::string("report handler failed: ") + thrown.what());
                } catch (...) {
                    print(id, "error", "report handler failed: unknown error");
                }
            }
            return nullptr;
        }
        if (setup)
            scene->setSetup(setup);
        if (onReport)
            scene->setReportHandler(onReport);
        return scene;
    });
}
