#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>

namespace Pens {

/**
 * @brief Configuration manager for SENS
 */
class Config {
public:
    static Config& getInstance();
    
    // Load configuration
    bool loadFromFile(const std::string& filename);
    bool loadFromEnv();
    
    // IMAP settings
    std::string getImapServer() const;
    int getImapPort() const;
    bool getImapUseSsl() const;
    std::string getImapUsername() const;
    std::string getImapPassword() const;
    
    // OAuth settings
    std::string getAuthMethod() const;  // "password" or "oauth"
    std::string getOAuthAccessToken() const;
    std::string getOAuthRefreshToken() const;
    bool useOAuth() const;
    
    // PENS settings
    int getPriorityThreshold() const;
    int getCheckInterval() const;
    bool getDebugMode() const;
    std::string getLogLevel() const;
    
    // Setters
    void setImapServer(const std::string& server);
    void setImapPort(int port);
    void setImapCredentials(const std::string& username, const std::string& password);
    void setPriorityThreshold(int level);
    
private:
    Config();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    std::map<std::string, std::string> config_;
    
    std::string getValue(const std::string& key, const std::string& defaultValue = "") const;
    int getValueInt(const std::string& key, int defaultValue = 0) const;
    bool getValueBool(const std::string& key, bool defaultValue = false) const;
};

} // namespace Pens

#endif // CONFIG_HPP

