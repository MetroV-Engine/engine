#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "Engine/Serialization/Serialization.hpp"

using Json = nlohmann::json;

inline Json parseJson(const std::string& text) {
    return Json::parse(text);
}

inline bool hasWarning(const serial::Report& report, const std::string& part) {
    for (const std::string& warning : report.warnings)
        if (warning.find(part) != std::string::npos)
            return true;
    return false;
}

inline bool hasError(const serial::Report& report, const std::string& part) {
    for (const std::string& error : report.errors)
        if (error.find(part) != std::string::npos)
            return true;
    return false;
}

// A world text with the given entities array, as the format writes it.
inline std::string worldText(const std::string& entities, int version = 1) {
    return R"({"format":"metrov.world","version":)" + std::to_string(version) + R"(,"entities":)" + entities + "}";
}

// Compares two values outside of boost::ut's operators, which cannot print containers.
template<typename T>
inline bool same(const T& a, const T& b) {
    return a == b;
}
