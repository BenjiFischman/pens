#include "config.hpp"
#include "logger.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace Pens {

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

Config::Config() {
    // Set default values
    config_["imap_server"] = "imap.gmail.com";
    config_["imap_port"] = "993";
    config_["imap_use_ssl"] = "true";
    config_["imap_username"] = "";
    config_["imap_password"] = "";
    config_["priority_threshold"] = "5";
    config_["check_interval"] = "60";
    config_["debug_mode"] = "false";
    config_["log_level"] = "INFO";
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open config file: " + filename);
        return false;
    }
    
    std::string line;
    int lineNum = 0;
    
    while (std::getline(file, line)) {
        lineNum++;
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse key=value
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            LOG_WARNING("Invalid config line " + std::to_string(lineNum) + ": " + line);
            continue;
        }
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // Remove quotes if present
        if (!value.empty() && value[0] == '"' && value[value.length()-1] == '"') {
            value = value.substr(1, value.length() - 2);
        }
        
        config_[key] = value;
    }
    
    LOG_INFO("Configuration loaded from file: " + filename);
    return true;
}

bool Config::loadFromEnv() {
    // Load from environment variables (support both PENS and legacy SENS prefix)
    const char* server = std::getenv("PENS_IMAP_SERVER");
    if (!server) server = std::getenv("SENS_IMAP_SERVER");  // Fallback
    if (server) config_["imap_server"] = server;
    
    const char* port = std::getenv("PENS_IMAP_PORT");
    if (!port) port = std::getenv("SENS_IMAP_PORT");
    if (port) config_["imap_port"] = port;
    
    const char* username = std::getenv("PENS_IMAP_USERNAME");
    if (!username) username = std::getenv("SENS_IMAP_USERNAME");
    if (username) config_["imap_username"] = username;
    
    const char* password = std::getenv("PENS_IMAP_PASSWORD");
    if (!password) password = std::getenv("SENS_IMAP_PASSWORD");
    if (password) config_["imap_password"] = password;
    
    const char* useSsl = std::getenv("PENS_IMAP_USE_SSL");
    if (useSsl) config_["imap_use_ssl"] = useSsl;
    
    const char* priorityThreshold = std::getenv("PENS_PRIORITY_THRESHOLD");
    if (priorityThreshold) config_["priority_threshold"] = priorityThreshold;
    
    const char* interval = std::getenv("PENS_CHECK_INTERVAL");
    if (interval) config_["check_interval"] = interval;
    
    const char* debug = std::getenv("PENS_DEBUG_MODE");
    if (debug) config_["debug_mode"] = debug;
    
    const char* logLevel = std::getenv("PENS_LOG_LEVEL");
    if (logLevel) config_["log_level"] = logLevel;
    
    // OAuth configuration
    const char* authMethod = std::getenv("PENS_AUTH_METHOD");
    if (authMethod) config_["auth_method"] = authMethod;
    
    const char* oauthAccessToken = std::getenv("PENS_OAUTH_ACCESS_TOKEN");
    if (oauthAccessToken) config_["oauth_access_token"] = oauthAccessToken;
    
    const char* oauthRefreshToken = std::getenv("PENS_OAUTH_REFRESH_TOKEN");
    if (oauthRefreshToken) config_["oauth_refresh_token"] = oauthRefreshToken;
    
    LOG_INFO("Configuration loaded from environment variables");
    return true;
}

std::string Config::getImapServer() const {
    return getValue("imap_server", "imap.gmail.com");
}

int Config::getImapPort() const {
    return getValueInt("imap_port", 993);
}

bool Config::getImapUseSsl() const {
    return getValueBool("imap_use_ssl", true);
}

std::string Config::getImapUsername() const {
    return getValue("imap_username", "");
}

std::string Config::getImapPassword() const {
    return getValue("imap_password", "");
}

int Config::getPriorityThreshold() const {
    return getValueInt("priority_threshold", 5);
}

int Config::getCheckInterval() const {
    return getValueInt("check_interval", 60);
}

bool Config::getDebugMode() const {
    return getValueBool("debug_mode", false);
}

std::string Config::getLogLevel() const {
    return getValue("log_level", "INFO");
}

std::string Config::getAuthMethod() const {
    return getValue("auth_method", "password");
}

std::string Config::getOAuthAccessToken() const {
    return getValue("oauth_access_token", "");
}

std::string Config::getOAuthRefreshToken() const {
    return getValue("oauth_refresh_token", "");
}

bool Config::useOAuth() const {
    std::string method = getAuthMethod();
    return (method == "oauth" || method == "OAuth" || method == "OAUTH");
}

void Config::setImapServer(const std::string& server) {
    config_["imap_server"] = server;
}

void Config::setImapPort(int port) {
    config_["imap_port"] = std::to_string(port);
}

void Config::setImapCredentials(const std::string& username, const std::string& password) {
    config_["imap_username"] = username;
    config_["imap_password"] = password;
}

void Config::setPriorityThreshold(int level) {
    config_["priority_threshold"] = std::to_string(level);
}

std::string Config::getValue(const std::string& key, const std::string& defaultValue) const {
    auto it = config_.find(key);
    return (it != config_.end()) ? it->second : defaultValue;
}

int Config::getValueInt(const std::string& key, int defaultValue) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return defaultValue;
    }
    
    try {
        return std::stoi(it->second);
    } catch (...) {
        return defaultValue;
    }
}

bool Config::getValueBool(const std::string& key, bool defaultValue) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return defaultValue;
    }
    
    std::string value = it->second;
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    return (value == "true" || value == "1" || value == "yes");
}

} // namespace Pens

