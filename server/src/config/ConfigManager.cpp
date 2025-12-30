#include "config/ConfigManager.hpp"

#include <fstream>
#include <iostream>

#include "spdlog/spdlog.h"

ConfigManager &ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() : m_initialized(false) {}

void ConfigManager::initialize(const std::string &configFilePath) {
    if (m_initialized) {
        spdlog::warn("ConfigManager already initialized. Skipping re-initialization");
        return;
    }

    std::ifstream file(configFilePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open configuration file: " + configFilePath);
    }

    try {
        json j;
        file >> j;

        m_config = j.get<AppConfig>();
        m_initialized = true;

        spdlog::info("[ConfigManager] Configuration loaded successfully from {}", configFilePath);
    } catch (const json::parse_error &e) {
        throw std::runtime_error("JSON parse error in " + configFilePath + ": " + e.what());
    } catch (const json::type_error &e) {
        throw std::runtime_error("JSON type mismatch in " + configFilePath + ": " + e.what());
    } catch (const std::exception &e) {
        throw std::runtime_error(
            "An unexpected error occurred while processing " + configFilePath + ": " + e.what());
    }
}

const AppConfig &ConfigManager::getConfig() const {
    if (!m_initialized) {
        throw std::runtime_error("ConfigManager Error: Configuration not initialized. Call initialize() first.");
    }
    return m_config;
}
