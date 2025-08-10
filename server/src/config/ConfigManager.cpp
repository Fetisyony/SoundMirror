#include "config/ConfigManager.hpp"

#include <fstream>
#include <iostream>

ConfigManager &ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() : m_initialized(false) {}

void ConfigManager::initialize(const std::string &configFilePath) {
    if (m_initialized) {
        std::cerr << "Warning: ConfigManager already initialized. Skipping re-initialization." << std::endl;
        return;
    }

    std::ifstream file(configFilePath);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigManager Error: Failed to open configuration file: " + configFilePath);
    }

    try {
        json j;
        file >> j;

        m_config = j.get<AppConfig>();
        m_initialized = true;

        std::cout << "ConfigManager: Configuration loaded successfully from " << configFilePath << std::endl;
    } catch (const json::parse_error &e) {
        throw std::runtime_error("ConfigManager Error: JSON parse error in " + configFilePath + ": " + e.what());
    } catch (const json::type_error &e) {
        throw std::runtime_error("ConfigManager Error: JSON type mismatch in " + configFilePath + ": " + e.what());
    } catch (const std::exception &e) {
        throw std::runtime_error(
            "ConfigManager Error: An unexpected error occurred while processing " + configFilePath + ": " + e.what());
    }
}

const AppConfig &ConfigManager::getConfig() const {
    if (!m_initialized) {
        throw std::runtime_error("ConfigManager Error: Configuration not initialized. Call initialize() first.");
    }
    return m_config;
}
