#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct NetworkConfig {
    int streamingPort;
    int timeSyncPort;
    int controlPort;
};

struct AppConfig {
    bool debugMode;
    NetworkConfig network;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NetworkConfig, streamingPort, timeSyncPort, controlPort)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AppConfig, debugMode, network)

class ConfigManager {
public:
    static ConfigManager &getInstance();

    void initialize(const std::string &configFilePath);

    [[nodiscard]] const AppConfig &getConfig() const;

    ConfigManager(const ConfigManager &) = delete;
    ConfigManager &operator=(const ConfigManager &) = delete;

private:
    ConfigManager();

    AppConfig m_config{};
    bool m_initialized;
};
