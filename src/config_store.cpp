#include "config_store.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <utility>

namespace ox::host {

namespace {

template <typename T>
T ReadScalar(const YAML::Node& node, T default_value) {
    if (!node || !node.IsScalar()) {
        return default_value;
    }

    try {
        return node.as<T>();
    } catch (const YAML::Exception&) {
        return default_value;
    }
}

}  // namespace

ConfigStore::ConfigStore(std::filesystem::path path) : path_(std::move(path)), root_(YAML::Node(YAML::NodeType::Map)) {
    try {
        if (std::filesystem::exists(path_)) {
            root_ = YAML::LoadFile(path_.string());
            if (!root_ || !root_.IsMap()) {
                root_ = YAML::Node(YAML::NodeType::Map);
            }
        }
    } catch (const YAML::Exception& e) {
        spdlog::warn("Failed to load config file {}: {}", path_.string(), e.what());
        root_ = YAML::Node(YAML::NodeType::Map);
    }
}

int ConfigStore::GetBool(const std::string& driver_namespace, const char* key, int default_value) {
    std::lock_guard<std::mutex> lock(mutex_);
    return ReadScalar<bool>(GetValueNodeLocked(driver_namespace, key), default_value != 0) ? 1 : 0;
}

int ConfigStore::GetInt(const std::string& driver_namespace, const char* key, int default_value) {
    std::lock_guard<std::mutex> lock(mutex_);
    return ReadScalar<int>(GetValueNodeLocked(driver_namespace, key), default_value);
}

float ConfigStore::GetFloat(const std::string& driver_namespace, const char* key, float default_value) {
    std::lock_guard<std::mutex> lock(mutex_);
    return ReadScalar<float>(GetValueNodeLocked(driver_namespace, key), default_value);
}

std::string ConfigStore::GetString(const std::string& driver_namespace, const char* key, const char* default_value) {
    std::lock_guard<std::mutex> lock(mutex_);
    return ReadScalar<std::string>(GetValueNodeLocked(driver_namespace, key), default_value ? default_value : "");
}

void ConfigStore::SetBool(const std::string& driver_namespace, const char* key, int value) {
    std::lock_guard<std::mutex> lock(mutex_);
    GetNamespaceNodeLocked(driver_namespace)[key] = value != 0;
    SaveLocked();
}

void ConfigStore::SetInt(const std::string& driver_namespace, const char* key, int value) {
    std::lock_guard<std::mutex> lock(mutex_);
    GetNamespaceNodeLocked(driver_namespace)[key] = value;
    SaveLocked();
}

void ConfigStore::SetFloat(const std::string& driver_namespace, const char* key, float value) {
    std::lock_guard<std::mutex> lock(mutex_);
    GetNamespaceNodeLocked(driver_namespace)[key] = value;
    SaveLocked();
}

void ConfigStore::SetString(const std::string& driver_namespace, const char* key, const char* value) {
    std::lock_guard<std::mutex> lock(mutex_);
    GetNamespaceNodeLocked(driver_namespace)[key] = value ? value : "";
    SaveLocked();
}

YAML::Node ConfigStore::GetNamespaceNodeLocked(const std::string& driver_namespace) {
    if (!root_ || !root_.IsMap()) {
        root_ = YAML::Node(YAML::NodeType::Map);
    }

    YAML::Node driver_node = root_[driver_namespace];
    if (!driver_node || !driver_node.IsMap()) {
        driver_node = root_[driver_namespace] = YAML::Node(YAML::NodeType::Map);
    }
    return driver_node;
}

YAML::Node ConfigStore::GetValueNodeLocked(const std::string& driver_namespace, const char* key) const {
    if (!root_ || !root_.IsMap() || !key || key[0] == '\0') {
        return {};
    }

    const YAML::Node driver_node = root_[driver_namespace];
    if (!driver_node || !driver_node.IsMap()) {
        return {};
    }

    return driver_node[key];
}

void ConfigStore::SaveLocked() {
    std::error_code error;
    const auto parent = path_.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
    }

    std::ofstream stream(path_, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        spdlog::warn("Failed to open config file {} for writing", path_.string());
        return;
    }

    YAML::Emitter emitter;
    emitter << root_;
    if (!emitter.good()) {
        spdlog::warn("Failed to serialize config file {}", path_.string());
        return;
    }

    stream << emitter.c_str();
}

}  // namespace ox::host