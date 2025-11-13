#ifndef OAUTH_TOKEN_MANAGER_HPP
#define OAUTH_TOKEN_MANAGER_HPP

#include "config.hpp"
#include <string>
#include <optional>

namespace Pens {

struct OAuthTokenData {
    std::string accessToken;
    std::string refreshToken;
    long expiresAt; // epoch seconds
    int expiresIn;  // seconds
};

class OAuthTokenManager {
public:
    explicit OAuthTokenManager(const Config& config);
    ~OAuthTokenManager();

    // Ensure we have a valid access token (refresh if necessary)
    bool ensureValidToken();

    // Access the current token (ensureValidToken must be called first)
    std::string getAccessToken() const;
    
    // Get certificate thumbprint (for Azure AD registration)
    std::string getCertificateThumbprint() const;

private:
    const Config& config_;
    std::string tokenFile_;
    std::string clientId_;
    std::string tenantId_;
    std::string scope_;
    std::string certificatePath_;
    std::string privateKeyPath_;
    std::string clientSecret_;

    OAuthTokenData token_{};
    bool tokenLoaded_ = false;

    bool loadTokenFromFile();
    bool saveTokenToFile() const;
    bool refreshAccessToken();
    bool acquireTokenWithCertificate();
    bool acquireTokenWithSecret();
    bool performTokenRequest(const std::string& tokenEndpoint, const std::string& postData);

    static bool parseJsonString(const std::string& json, const std::string& key, std::string& value);
    static bool parseJsonInt(const std::string& json, const std::string& key, long& value);
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

} // namespace Pens

#endif // OAUTH_TOKEN_MANAGER_HPP
