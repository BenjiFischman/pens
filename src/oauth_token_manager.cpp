#include "oauth_token_manager.hpp"
#include "oauth_helper.hpp"
#include "logger.hpp"
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <curl/curl.h>

namespace Pens {

OAuthTokenManager::OAuthTokenManager(const Config& config)
    : config_(config)
    , tokenFile_(config.getOAuthTokenFile())
    , clientId_(config.getOAuthClientId())
    , tenantId_(config.getOAuthTenantId())
    , scope_(config.getOAuthScope())
    , certificatePath_(config.getOAuthCertificatePath())
    , privateKeyPath_(config.getOAuthPrivateKeyPath())
    , clientSecret_(config.getOAuthClientSecret()) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

OAuthTokenManager::~OAuthTokenManager() {
    curl_global_cleanup();
}

bool OAuthTokenManager::ensureValidToken() {
    if (!tokenLoaded_) {
        if (!loadTokenFromFile()) {
            LOG_ERROR("Failed to load OAuth token file: " + tokenFile_);
            return false;
        }
        tokenLoaded_ = true;
    }

    long currentTime = std::time(nullptr);
    if (token_.accessToken.empty()) {
        LOG_ERROR("OAuth access token missing in token file");
        return false;
    }

    if (token_.refreshToken.empty()) {
        if (token_.expiresAt == 0 || currentTime < token_.expiresAt) {
            return true;
        }
        LOG_ERROR("OAuth refresh token missing; token cannot be renewed");
        return false;
    }

    // Refresh if expired or will expire in <5 minutes
    if (token_.expiresAt != 0 && currentTime < token_.expiresAt - 300) {
        return true;
    }

    LOG_INFO("OAuth access token expired or expiring soon; refreshing...");
    if (!refreshAccessToken()) {
        LOG_ERROR("OAuth refresh token flow failed");
        return false;
    }

    return true;
}

std::string OAuthTokenManager::getAccessToken() const {
    return token_.accessToken;
}

std::string OAuthTokenManager::getCertificateThumbprint() const {
    if (certificatePath_.empty()) {
        return "";
    }
    return OAuthHelper::calculateCertificateThumbprint(certificatePath_);
}

bool OAuthTokenManager::loadTokenFromFile() {
    std::ifstream file(tokenFile_);
    if (!file.is_open()) {
        LOG_ERROR("Could not open OAuth token file: " + tokenFile_);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    if (!parseJsonString(json, "access_token", token_.accessToken)) {
        LOG_ERROR("access_token not found in token file");
        return false;
    }

    parseJsonString(json, "refresh_token", token_.refreshToken);

    long expiresIn = 0;
    long acquiredAt = 0;
    long expiresAt = 0;

    if (parseJsonInt(json, "expires_in", expiresIn)) {
        token_.expiresIn = static_cast<int>(expiresIn);
    } else {
        token_.expiresIn = 3600;
    }

    if (parseJsonInt(json, "acquired_at", acquiredAt)) {
        if (acquiredAt > 1000000000000L) { // milliseconds
            acquiredAt /= 1000;
        }
        token_.expiresAt = acquiredAt + token_.expiresIn;
    } else if (parseJsonInt(json, "expires_at", expiresAt)) {
        if (expiresAt > 1000000000000L) {
            expiresAt /= 1000;
        }
        token_.expiresAt = expiresAt;
    } else {
        token_.expiresAt = 0;
    }

    LOG_INFO("OAuth token loaded. Expires at: " + std::to_string(token_.expiresAt));
    return true;
}

bool OAuthTokenManager::saveTokenToFile() const {
    std::ofstream file(tokenFile_, std::ios::trunc);
    if (!file.is_open()) {
        LOG_ERROR("Failed to write OAuth token file: " + tokenFile_);
        return false;
    }

    long acquiredAt = std::time(nullptr);
    file << "{\n"
         << "  \"access_token\": \"" << token_.accessToken << "\",\n"
         << "  \"refresh_token\": \"" << token_.refreshToken << "\",\n"
         << "  \"expires_in\": " << token_.expiresIn << ",\n"
         << "  \"acquired_at\": " << acquiredAt << "\n"
         << "}\n";

    LOG_INFO("OAuth token updated and saved to file");
    return true;
}

bool OAuthTokenManager::refreshAccessToken() {
    if (clientId_.empty()) {
        LOG_ERROR("OAuth client_id not configured. Set PENS_OAUTH_CLIENT_ID or update config file.");
        return false;
    }

    if (tenantId_.empty()) {
        LOG_ERROR("OAuth tenant_id not configured. Set PENS_OAUTH_TENANT_ID or update config file.");
        return false;
    }

    // Use certificate if available, otherwise fall back to client secret
    if (!certificatePath_.empty() && !privateKeyPath_.empty()) {
        LOG_INFO("Using certificate-based authentication for token refresh");
        LOG_DEBUG("Certificate path: " + certificatePath_);
        LOG_DEBUG("Private key path: " + privateKeyPath_);
        return acquireTokenWithCertificate();
    } else if (!clientSecret_.empty()) {
        LOG_INFO("Using client secret for token refresh");
        return acquireTokenWithSecret();
    } else {
        LOG_ERROR("Neither certificate nor client secret configured for token refresh");
        LOG_ERROR("Certificate path: " + (certificatePath_.empty() ? "(empty)" : certificatePath_));
        LOG_ERROR("Private key path: " + (privateKeyPath_.empty() ? "(empty)" : privateKeyPath_));
        LOG_ERROR("Client secret: " + std::string(clientSecret_.empty() ? "(empty)" : "(configured)"));
        return false;
    }
}

bool OAuthTokenManager::acquireTokenWithCertificate() {
    std::string tokenEndpoint = "https://login.microsoftonline.com/" + tenantId_ + "/oauth2/v2.0/token";
    
    LOG_DEBUG("Generating client assertion JWT...");
    LOG_DEBUG("Client ID: " + clientId_);
    LOG_DEBUG("Tenant ID: " + tenantId_);
    LOG_DEBUG("Certificate: " + certificatePath_);
    LOG_DEBUG("Private Key: " + privateKeyPath_);
    
    // Generate client assertion JWT
    std::string clientAssertion = OAuthHelper::generateClientAssertion(
        clientId_, tenantId_, certificatePath_, privateKeyPath_
    );
    
    if (clientAssertion.empty()) {
        LOG_ERROR("Failed to generate client assertion");
        LOG_ERROR("Check that certificate and private key files exist and are readable");
        return false;
    }
    
    LOG_DEBUG("Client assertion generated successfully (length: " + std::to_string(clientAssertion.length()) + ")");
    
    std::string postData = "client_id=" + OAuthHelper::urlEncode(clientId_)
        + "&grant_type=refresh_token"
        + "&refresh_token=" + OAuthHelper::urlEncode(token_.refreshToken)
        + "&client_assertion_type=urn:ietf:params:oauth:client-assertion-type:jwt-bearer"
        + "&client_assertion=" + OAuthHelper::urlEncode(clientAssertion);

    if (!scope_.empty()) {
        postData += "&scope=" + OAuthHelper::urlEncode(scope_);
    }
    
    LOG_DEBUG("Sending token request with client assertion...");
    return performTokenRequest(tokenEndpoint, postData);
}

bool OAuthTokenManager::acquireTokenWithSecret() {
    std::string tokenEndpoint = "https://login.microsoftonline.com/" + tenantId_ + "/oauth2/v2.0/token";
    std::string postData = "client_id=" + OAuthHelper::urlEncode(clientId_)
        + "&grant_type=refresh_token"
        + "&refresh_token=" + OAuthHelper::urlEncode(token_.refreshToken)
        + "&client_secret=" + OAuthHelper::urlEncode(clientSecret_);

    if (!scope_.empty()) {
        postData += "&scope=" + OAuthHelper::urlEncode(scope_);
    }
    
    return performTokenRequest(tokenEndpoint, postData);
}

bool OAuthTokenManager::performTokenRequest(const std::string& tokenEndpoint, const std::string& postData) {

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize CURL for OAuth refresh");
        return false;
    }

    std::string responseBody;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, tokenEndpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postData.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OAuthTokenManager::writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("CURL error during OAuth refresh: " + std::string(curl_easy_strerror(res)));
        return false;
    }

    if (httpCode < 200 || httpCode >= 300) {
        std::string errorCode;
        std::string errorDescription;
        
        parseJsonString(responseBody, "error", errorCode);
        parseJsonString(responseBody, "error_description", errorDescription);
        
        LOG_ERROR("OAuth refresh failed with HTTP status: " + std::to_string(httpCode));
        
        if (!errorCode.empty()) {
            LOG_ERROR("Azure AD Error Code: " + errorCode);
            
            // Provide helpful messages for common errors
            if (errorCode == "AADSTS700016") {
                LOG_ERROR("Application not found in tenant. Possible causes:");
                LOG_ERROR("  1. Client ID is incorrect: " + clientId_);
                LOG_ERROR("  2. Application is not registered in tenant: " + tenantId_);
                LOG_ERROR("  3. Tenant ID does not match the tenant where the app is registered");
                LOG_ERROR("  Solution: Verify the Client ID and Tenant ID in your config file");
            } else if (errorCode == "AADSTS7000215") {
                LOG_ERROR("Invalid client secret provided");
            } else if (errorCode == "AADSTS7000218") {
                LOG_ERROR("Client assertion or client secret required but not provided");
                LOG_ERROR("Possible causes:");
                LOG_ERROR("  1. Certificate not uploaded to Azure AD");
                LOG_ERROR("  2. Certificate thumbprint doesn't match Azure AD");
                LOG_ERROR("  3. Certificate file cannot be read");
                LOG_ERROR("  4. Private key file cannot be read");
                LOG_ERROR("  5. Client assertion generation failed");
                LOG_ERROR("Solution:");
                LOG_ERROR("  - Verify certificate is uploaded to Azure AD");
                LOG_ERROR("  - Check certificate paths in config file");
                LOG_ERROR("  - Run: node scripts/verify-certificate.js");
            } else if (errorCode == "AADSTS70011") {
                LOG_ERROR("Invalid scope. Check that your scopes are correct: " + scope_);
            } else if (errorCode == "AADSTS50173") {
                LOG_ERROR("Fresh authentication required. Please re-run oauth-token-helper.js");
            } else if (errorCode == "invalid_grant" || errorCode == "AADSTS40016") {
                LOG_ERROR("Invalid grant - refresh token may be expired or invalid");
                LOG_ERROR("Possible causes:");
                LOG_ERROR("  1. Refresh token has expired (refresh tokens can expire after 90 days of inactivity)");
                LOG_ERROR("  2. Refresh token was revoked");
                LOG_ERROR("  3. Certificate thumbprint doesn't match Azure AD");
                LOG_ERROR("  4. JWT signature verification failed (certificate/key mismatch)");
                LOG_ERROR("  5. Refresh token was obtained with different authentication method");
                LOG_ERROR("Solution:");
                LOG_ERROR("  - Re-run oauth-token-helper.js or oauth-auth-code-helper.js to get a new token");
                LOG_ERROR("  - Verify certificate thumbprint matches Azure AD");
                LOG_ERROR("  - Run: node scripts/verify-certificate.js");
                LOG_ERROR("  - Ensure certificate and private key match");
            }
        }
        
        if (!errorDescription.empty()) {
            LOG_ERROR("Error Description: " + errorDescription);
        } else {
            LOG_ERROR("Response: " + responseBody);
        }
        
        return false;
    }

    std::string newAccessToken;
    if (!parseJsonString(responseBody, "access_token", newAccessToken)) {
        LOG_ERROR("OAuth refresh response missing access_token");
        return false;
    }

    std::string newRefreshToken;
    if (parseJsonString(responseBody, "refresh_token", newRefreshToken) && !newRefreshToken.empty()) {
        token_.refreshToken = newRefreshToken;
    }

    long expiresIn = 0;
    if (parseJsonInt(responseBody, "expires_in", expiresIn)) {
        token_.expiresIn = static_cast<int>(expiresIn);
    } else {
        token_.expiresIn = 3600;
    }

    token_.accessToken = newAccessToken;
    token_.expiresAt = std::time(nullptr) + token_.expiresIn;

    return saveTokenToFile();
}

bool OAuthTokenManager::parseJsonString(const std::string& json, const std::string& key, std::string& value) {
    std::string pattern = "\"" + key + "\"";
    size_t keyPos = json.find(pattern);
    if (keyPos == std::string::npos) {
        return false;
    }
    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) {
        return false;
    }
    size_t quoteStart = json.find('"', colonPos + 1);
    if (quoteStart == std::string::npos) {
        return false;
    }
    size_t quoteEnd = json.find('"', quoteStart + 1);
    if (quoteEnd == std::string::npos) {
        return false;
    }
    value = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
    return true;
}

bool OAuthTokenManager::parseJsonInt(const std::string& json, const std::string& key, long& value) {
    std::string pattern = "\"" + key + "\"";
    size_t keyPos = json.find(pattern);
    if (keyPos == std::string::npos) {
        return false;
    }
    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) {
        return false;
    }
    size_t numberStart = json.find_first_of("-0123456789", colonPos + 1);
    if (numberStart == std::string::npos) {
        return false;
    }
    size_t numberEnd = json.find_first_not_of("0123456789", numberStart);
    std::string numberStr = json.substr(numberStart, numberEnd - numberStart);
    try {
        value = std::stol(numberStr);
        return true;
    } catch (...) {
        return false;
    }
}

size_t OAuthTokenManager::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

} // namespace Pens
