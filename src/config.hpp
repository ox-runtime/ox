#pragma once

#include <ox_driver.h>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <string>
#include <string_view>

namespace ox {

inline bool LoadConfig(const std::filesystem::path& config_path, YAML::Node& out_config) {
    if (!std::filesystem::exists(config_path)) {
        spdlog::error("Config file '{}' does not exist.", config_path.string());
        return false;
    }

    try {
        out_config = YAML::LoadFile(config_path.string());
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error loading config file '{}': {}", config_path.string(), e.what());
        return false;
    }
}

inline void ApplyScalarNode(const YAML::Node& node, OxDriver* d, const std::string& k) {
    if (!d) return;

    // try bool
    bool boolVal;
    if (YAML::convert<bool>::decode(node, boolVal)) {
        if (d->set_config_bool) d->set_config_bool(k.c_str(), boolVal ? XR_TRUE : XR_FALSE);
        return;
    }

    // try int
    int64_t intVal;
    if (YAML::convert<int64_t>::decode(node, intVal)) {
        if (d->set_config_int) d->set_config_int(k.c_str(), intVal);
        return;
    }

    // try float
    float floatVal;
    if (YAML::convert<float>::decode(node, floatVal)) {
        if (d->set_config_float) d->set_config_float(k.c_str(), floatVal);
        return;
    }

    // fallback to string
    if (d->set_config_string) d->set_config_string(k.c_str(), node.Scalar().c_str());
}

inline void WalkNode(const YAML::Node& node, const std::string& prefix, OxDriver* d) {
    if (node.IsMap()) {
        for (const auto& entry : node) {
            if (!entry.first.IsScalar()) continue;
            const std::string k = prefix.empty() ? entry.first.Scalar() : prefix + "." + entry.first.Scalar();
            WalkNode(entry.second, k, d);
        }
    } else if (node.IsScalar()) {
        ApplyScalarNode(node, d, prefix);
    }
}

inline void ApplyDriverConfig(const YAML::Node& config, std::string_view driver_name, OxDriver* driver) {
    if (!config) return;

    const YAML::Node drivers = config["drivers"];
    if (!drivers || !drivers.IsMap()) return;

    const YAML::Node driver_config = drivers[std::string(driver_name)];
    if (!driver_config || !driver_config.IsMap()) return;

    WalkNode(driver_config, "", driver);
}

}  // namespace ox