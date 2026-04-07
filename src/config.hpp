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

    const std::string& tag = node.Tag();

    if (tag == "tag:yaml.org,2002:bool") {
        if (d->set_config_bool) d->set_config_bool(k.c_str(), node.as<bool>() ? XR_TRUE : XR_FALSE);
        return;
    }
    if (tag == "tag:yaml.org,2002:int") {
        if (d->set_config_int) d->set_config_int(k.c_str(), node.as<int64_t>());
        return;
    }
    if (tag == "tag:yaml.org,2002:float") {
        if (d->set_config_float) d->set_config_float(k.c_str(), node.as<float>());
        return;
    }
    // tag:yaml.org,2002:str or unresolved/plain — treat as string
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