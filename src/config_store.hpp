#pragma once

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <mutex>
#include <string>

namespace ox::host {

class ConfigStore {
   public:
    explicit ConfigStore(std::filesystem::path path);

    int GetBool(const std::string& driver_namespace, const char* key, int default_value);
    int GetInt(const std::string& driver_namespace, const char* key, int default_value);
    float GetFloat(const std::string& driver_namespace, const char* key, float default_value);
    std::string GetString(const std::string& driver_namespace, const char* key, const char* default_value);

    void SetBool(const std::string& driver_namespace, const char* key, int value);
    void SetInt(const std::string& driver_namespace, const char* key, int value);
    void SetFloat(const std::string& driver_namespace, const char* key, float value);
    void SetString(const std::string& driver_namespace, const char* key, const char* value);

   private:
    YAML::Node GetNamespaceNodeLocked(const std::string& driver_namespace);
    YAML::Node GetValueNodeLocked(const std::string& driver_namespace, const char* key) const;
    void SaveLocked();

    std::filesystem::path path_;
    YAML::Node root_;
    std::mutex mutex_;
};

}  // namespace ox::host