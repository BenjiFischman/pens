#ifndef OAUTH_HELPER_HPP
#define OAUTH_HELPER_HPP

#include <string>
#include <map>

namespace Pens {

/**
 * @brief OAuth 2.0 Helper for Microsoft 365 Authentication
 * 
 * Handles OAuth 2.0 token generation and XOAUTH2 SASL format
 */
class OAuthHelper {
public:
    /**
     * @brief Generate XOAUTH2 auth string
     * 
     * Format: base64(user={email}\x01auth=Bearer {token}\x01\x01)
     * 
     * @param email User email address
     * @param accessToken OAuth 2.0 access token
     * @return Base64 encoded XOAUTH2 string
     */
    static std::string generateXOAuth2String(
        const std::string& email, 
        const std::string& accessToken
    );
    
    /**
     * @brief Parse OAuth token from JSON response
     * 
     * @param jsonResponse JSON string from token endpoint
     * @return Access token or empty string on error
     */
    static std::string parseAccessToken(const std::string& jsonResponse);
    
    /**
     * @brief Check if token is likely expired
     * 
     * @param tokenTimestamp When token was acquired (Unix timestamp)
     * @param expiresIn Token lifetime in seconds (default: 3600)
     * @return true if token likely expired
     */
    static bool isTokenExpired(long tokenTimestamp, int expiresIn = 3600);
    
    /**
     * @brief Build OAuth 2.0 token request URL for Microsoft
     * 
     * @param clientId Azure AD application client ID
     * @param tenantId Azure AD tenant ID (or "common")
     * @param redirectUri Redirect URI (must match Azure AD config)
     * @param scope Space-separated scopes (e.g., "https://outlook.office365.com/.default")
     * @return Authorization URL for user to visit
     */
    static std::string buildAuthorizationUrl(
        const std::string& clientId,
        const std::string& tenantId,
        const std::string& redirectUri,
        const std::string& scope
    );
    
private:
    static std::string base64Encode(const std::string& input);
    static std::string urlEncode(const std::string& value);
};

} // namespace Pens

#endif // OAUTH_HELPER_HPP

