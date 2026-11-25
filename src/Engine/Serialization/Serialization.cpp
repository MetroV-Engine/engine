#include "Engine/Serialization/Serialization.hpp"

#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#endif

#include "ECS/Reflection/Reflection.hpp"
#include "Engine/Serialization/detail/ValueCodec.hpp"

namespace serial {
    using namespace ECS;   // this file only: the ECS types (Registry, Entity, reflect...) without prefix

    namespace {
        using detail::Json;

        constexpr const char* FormatName = "metrov.world";

        // Collects into the caller's report, or into a throwaway one.
        class ReportScope {
            public:
                explicit ReportScope(Report* report) : _report(report ? *report : _local) {}
                Report& get() { return _report; }

            private:
                Report _local;
                Report& _report;
        };

        // registerType returns the type already registered by Engine/Components/Uuid.hpp, and never null.
        const reflect::TypeInfo& uuidType() {
            return reflect::TypeRegistry::instance().registerType<Uuid>();
        }

        // The Uuid component of an entity, or a null Uuid.
        Uuid uuidOf(reflect::RegistryInspector& inspector, Entity entity) {
            const reflect::Ref ref = inspector.get(entity, uuidType());
            const Uuid* uuid = ref.valid() ? ref.tryAs<Uuid>() : nullptr;
            return uuid ? *uuid : Uuid{};
        }

        bool parse(std::string_view text, Json& out, Report& report) {
            try {
                out = Json::parse(text);
                return true;
            } catch (const std::exception& error) {
                report.errors.push_back(std::string("invalid text: ") + error.what());
                return false;
            }
        }

        // One entity of the text, checked but not created yet.
        struct PendingEntity {
            Uuid id;
            const Json* components = nullptr;   // null: none
        };

        // Reads the optional "systems" block. Not an array: an error. An entry
        // that is not a non-empty string: skipped with a warning.
        bool readSystems(const Json& doc, std::vector<std::string>& systems, Report& report) {
            if (!doc.contains("systems")) {
                return true;
            }
            const Json& block = doc["systems"];
            if (!block.is_array()) {
                report.errors.push_back("'systems' must be an array of system names");
                return false;
            }
            std::size_t position = 0;
            for (const Json& entry : block) {
                const std::string where = "systems[" + std::to_string(position++) + "]";
                if (!entry.is_string() || entry.get<std::string>().empty()) {
                    report.warnings.push_back(where + ": a system name must be a non-empty string, skipped");
                    continue;
                }
                systems.push_back(entry.get<std::string>());
            }
            return true;
        }

        // Checks the whole text before anything is created. Returns false and
        // reports every problem found.
        bool validate(const Json& doc, std::vector<PendingEntity>& pending, const std::unordered_set<Uuid>& existing,
                      std::vector<std::string>& systems, Report& report) {
            if (!doc.is_object() || !doc.contains("format") || doc["format"] != FormatName) {
                report.errors.push_back(std::string("not a '") + FormatName + "' text");
                return false;
            }
            if (!doc.contains("version") || !doc["version"].is_number_integer()) {
                report.errors.push_back("the format version is missing");
                return false;
            }
            const auto version = doc["version"].get<long long>();
            if (version < 1 || version > FormatVersion) {
                report.errors.push_back("unsupported format version " + std::to_string(version) +
                                        " (this build reads up to " + std::to_string(FormatVersion) + ")");
                return false;
            }
            if (!doc.contains("entities") || !doc["entities"].is_array()) {
                report.errors.push_back("'entities' must be an array");
                return false;
            }

            bool valid = readSystems(doc, systems, report);
            std::unordered_set<Uuid> seen;
            std::size_t position = 0;
            for (const Json& item : doc["entities"]) {
                const std::string where = "entities[" + std::to_string(position++) + "]";
                if (!item.is_object() || !item.contains("id") || !item["id"].is_string()) {
                    report.errors.push_back(where + ": an entity needs an 'id' string");
                    valid = false;
                    continue;
                }
                const std::optional<Uuid> id = uuid::fromString(item["id"].get<std::string>());
                if (!id || uuid::isNull(*id)) {
                    report.errors.push_back(where + ": '" + item["id"].get<std::string>() + "' is not a valid id");
                    valid = false;
                    continue;
                }
                if (!seen.insert(*id).second) {
                    report.errors.push_back(where + ": the id " + uuid::toString(*id) + " appears twice in the text");
                    valid = false;
                    continue;
                }
                if (existing.count(*id)) {
                    report.errors.push_back(where + ": the id " + uuid::toString(*id) + " is already in the world");
                    valid = false;
                    continue;
                }
                PendingEntity entity{*id, nullptr};
                if (item.contains("components")) {
                    if (!item["components"].is_object()) {
                        report.errors.push_back(where + ": 'components' must be an object");
                        valid = false;
                        continue;
                    }
                    entity.components = &item["components"];
                }
                pending.push_back(entity);
            }
            return valid;
        }

        // Writes text to file and makes sure it reached the disk.
        bool writeDurably(const std::filesystem::path& file, const std::string& text) {
            std::FILE* out = std::fopen(file.string().c_str(), "wb");
            if (!out) {
                return false;
            }
            bool ok = std::fwrite(text.data(), 1, text.size(), out) == text.size() && std::fflush(out) == 0;
#if defined(__unix__) || defined(__APPLE__)
            ok = ok && ::fsync(::fileno(out)) == 0;
#endif
            ok = (std::fclose(out) == 0) && ok;   // a close error is a write error too
            return ok;
        }

        // Best effort: makes the rename itself survive a crash.
        void syncDirectory(const std::filesystem::path& file) {
#if defined(__unix__) || defined(__APPLE__)
            std::filesystem::path directory = file.parent_path();
            if (directory.empty()) {
                directory = ".";
            }
            const int descriptor = ::open(directory.string().c_str(), O_RDONLY);
            if (descriptor >= 0) {
                ::fsync(descriptor);
                ::close(descriptor);
            }
#else
            (void)file;
#endif
        }

        bool readFile(const std::filesystem::path& path, std::string& out, Report& report) {
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                report.errors.push_back("cannot open '" + path.string() + "' for reading");
                return false;
            }
            std::ostringstream buffer;
            buffer << file.rdbuf();
            if (file.bad()) {
                report.errors.push_back("cannot read '" + path.string() + "'");
                return false;
            }
            out = buffer.str();
            return true;
        }
    }

    std::string serialize(Registry& world, Report* report, const SaveOptions& options, const SceneInfo* scene) {
        ReportScope scope(report);
        Report& rep = scope.get();
        try {
            reflect::RegistryInspector inspector = world.inspect();

            // Pass 1: the id of every entity. One without a Uuid (or whose
            // Uuid is already taken) gets a temporary one, in the text only,
            // unless options.assignMissingUuids makes it the entity's own.
            std::map<Entity, Uuid> ids;
            std::vector<Entity> order;
            std::unordered_set<Uuid> used;
            for (const Entity entity : inspector.entities()) {
                Uuid id = uuidOf(inspector, entity);
                bool assignable = uuid::isNull(id);
                if (!assignable && used.count(id)) {
                    rep.warnings.push_back(inspector.displayName(entity) + ": the Uuid " + uuid::toString(id) +
                                           " is used by another entity, a temporary one is written instead");
                    id = Uuid{};
                }
                while (uuid::isNull(id) || used.count(id)) {
                    id = uuid::generate();
                }
                if (assignable && options.assignMissingUuids) {
                    world.addComponent(entity, Uuid{id});
                }
                used.insert(id);
                ids.emplace(entity, id);
                order.push_back(entity);
            }

            // Pass 2: the components.
            Json entities = Json::array();
            detail::EncodeContext context{rep, &ids};
            for (const Entity entity : order) {
                const std::string id = uuid::toString(ids.at(entity));
                Json components = Json::object();
                for (const reflect::Ref component : inspector.list(entity)) {
                    if (&component.type() == &uuidType()) {
                        continue;   // it is the entity's id, not a component of the text
                    }
                    if (auto json = detail::encode(component, context, id + ":" + component.type().name)) {
                        components[component.type().name] = std::move(*json);
                    }
                }
                Json item = Json::object();
                item["id"] = id;
                item["components"] = std::move(components);
                entities.push_back(std::move(item));
            }

            Json doc = Json::object();
            doc["format"] = FormatName;
            doc["version"] = FormatVersion;
            if (scene) {
                doc["systems"] = scene->systems;   // before the entities: short, read first by a human
            }
            doc["entities"] = std::move(entities);
            return doc.dump(2) + "\n";
        } catch (const std::exception& error) {
            rep.errors.push_back(std::string("cannot serialize the world: ") + error.what());
            return {};
        }
    }

    bool load(Registry& world, std::string_view text, Report* report, SceneInfo* scene) {
        ReportScope scope(report);
        Report& rep = scope.get();
        const std::size_t errorsBefore = rep.errors.size();   // the Report may hold the errors of earlier calls

        Json doc;
        if (!parse(text, doc, rep)) {
            return false;
        }

        reflect::RegistryInspector inspector = world.inspect();
        std::unordered_set<Uuid> existing;
        for (const Entity entity : inspector.entities()) {
            const Uuid id = uuidOf(inspector, entity);
            if (!uuid::isNull(id)) {
                existing.insert(id);
            }
        }

        std::vector<PendingEntity> pending;
        std::vector<std::string> systems;
        if (!validate(doc, pending, existing, systems, rep)) {
            return false;
        }

        std::vector<Entity> created;
        try {
            // Pass 1: every entity exists before any component is read, so a
            // reference can point to an entity that comes later in the text.
            std::unordered_map<Uuid, Entity> entities;
            for (const PendingEntity& item : pending) {
                const Entity entity = world.spawnEntity();
                created.push_back(entity);
                world.addComponent(entity, Uuid{item.id});
                entities.emplace(item.id, entity);
            }

            // Pass 2: the components.
            detail::DecodeContext context{rep, &entities};
            for (std::size_t i = 0; i < pending.size(); ++i) {
                if (!pending[i].components) {
                    continue;
                }
                const Json& components = *pending[i].components;
                for (auto it = components.begin(); it != components.end(); ++it) {
                    const std::string path = uuid::toString(pending[i].id) + ":" + it.key();
                    const reflect::TypeInfo* type = reflect::RegistryInspector::findType(it.key());
                    if (!type) {
                        rep.warnings.push_back(path + ": unknown component, skipped");
                        continue;
                    }
                    if (type == &uuidType()) {
                        rep.warnings.push_back(path + ": the id of an entity is not a component, skipped");
                        continue;
                    }
                    const reflect::Ref ref = inspector.add(created[i], *type);
                    if (!ref) {
                        rep.warnings.push_back(path + ": cannot be created (no default constructor), skipped");
                        continue;
                    }
                    detail::decode(it.value(), ref, context, path);
                }
            }
            if (rep.errors.size() != errorsBefore) {
                return false;
            }
            if (scene) {
                scene->systems = std::move(systems);
            }
            return true;
        } catch (const std::exception& error) {
            for (const Entity entity : created) {
                if (world.isAlive(entity)) {
                    world.killEntity(entity);
                }
            }
            rep.errors.push_back(std::string("cannot load the world: ") + error.what());
            return false;
        }
    }

    bool saveFile(Registry& world, const std::filesystem::path& path, Report* report, const SaveOptions& options,
                  const SceneInfo* scene) {
        ReportScope scope(report);
        Report& rep = scope.get();
        const std::size_t errorsBefore = rep.errors.size();   // the Report may hold the errors of earlier calls

        const std::string text = serialize(world, &rep, options, scene);
        if (rep.errors.size() != errorsBefore || text.empty()) {
            return false;
        }

        // A temporary file of its own name (two saves at once do not meet),
        // flushed to the disk, then renamed over the target.
        std::filesystem::path temporary = path;
        temporary.replace_filename(path.filename().string() + "." + uuid::toString(uuid::generate()) + ".tmp");
        std::error_code error;
        if (!writeDurably(temporary, text)) {
            rep.errors.push_back("cannot write '" + temporary.string() + "'");
            std::filesystem::remove(temporary, error);
            return false;
        }
        std::filesystem::rename(temporary, path, error);
        if (error) {
            rep.errors.push_back("cannot replace '" + path.string() + "': " + error.message());
            std::filesystem::remove(temporary, error);
            return false;
        }
        syncDirectory(path);
        return true;
    }

    bool loadFile(Registry& world, const std::filesystem::path& path, Report* report, SceneInfo* scene) {
        ReportScope scope(report);
        std::string text;
        if (!readFile(path, text, scope.get())) {
            return false;
        }
        return load(world, text, &scope.get(), scene);
    }

    std::string serialize(reflect::Ref value, Report* report) {
        ReportScope scope(report);
        Report& rep = scope.get();
        if (!value.valid()) {
            rep.errors.push_back("cannot serialize an invalid Ref");
            return {};
        }
        try {
            detail::EncodeContext context{rep, nullptr};
            const std::optional<Json> json = detail::encode(value, context, value.type().name);
            if (!json) {
                rep.errors.push_back("type '" + value.type().name + "' is not serializable");
                return {};
            }
            return json->dump(2) + "\n";
        } catch (const std::exception& error) {
            rep.errors.push_back(std::string("cannot serialize the value: ") + error.what());
            return {};
        }
    }

    bool load(reflect::Ref target, std::string_view text, Report* report) {
        ReportScope scope(report);
        Report& rep = scope.get();
        if (!target.valid()) {
            rep.errors.push_back("cannot load into an invalid Ref");
            return false;
        }
        Json json;
        if (!parse(text, json, rep)) {
            return false;
        }
        try {
            detail::DecodeContext context{rep, nullptr};
            detail::decode(json, target, context, target.type().name);
            return true;
        } catch (const std::exception& error) {
            rep.errors.push_back(std::string("cannot load the value: ") + error.what());
            return false;
        }
    }
}
