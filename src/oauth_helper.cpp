#include "oauth_helper.hpp"
#include "logger.hpp"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

namespace Pens {

// Base64 encoding
std::string OAuthHelper::base64Encode(const std::string& input) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    
    BIO_write(bio, input.c_str(), input.length());
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    
    return result;
}

// URL encoding
std::string OAuthHelper::urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        // Keep alphanumeric and other safe characters
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            // Percent-encode everything else
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }

    return escaped.str();
}

std::string OAuthHelper::generateXOAuth2String(
    const std::string& email,
    const std::string& accessToken
) {
    // XOAUTH2 SASL format:
    // base64(user={email}\x01auth=Bearer {token}\x01\x01)
    
    std::ostringstream authString;
    authString << "user=" << email << "\x01"
               << "auth=Bearer " << accessToken << "\x01\x01";
    
    std::string encoded = base64Encode(authString.str());
    
    LOG_DEBUG("Generated XOAUTH2 string for: " + email);
    
    return encoded;
}

std::string OAuthHelper::parseAccessToken(const std::string& jsonResponse) {
    // Simple JSON parsing for access_token field
    // Format: {"access_token":"TOKEN","token_type":"Bearer",...}
    
    const std::string tokenKey = "\"access_token\":\"";
    size_t tokenStart = jsonResponse.find(tokenKey);
    
    if (tokenStart == std::string::npos) {
        LOG_ERROR("Could not find access_token in response");
        return "";
    }
    
    tokenStart += tokenKey.length();
    size_t tokenEnd = jsonResponse.find("\"", tokenStart);
    
    if (tokenEnd == std::string::npos) {
        LOG_ERROR("Malformed access_token in response");
        return "";
    }
    
    std::string token = jsonResponse.substr(tokenStart, tokenEnd - tokenStart);
    LOG_INFO("Successfully parsed access token");
    
    return token;
}

bool OAuthHelper::isTokenExpired(long tokenTimestamp, int expiresIn) {
    long currentTime = std::time(nullptr);
    long elapsedSeconds = currentTime - tokenTimestamp;
    
    // Add 5-minute buffer to avoid edge cases
    return elapsedSeconds >= (expiresIn - 300);
}

std::string OAuthHelper::buildAuthorizationUrl(
    const std::string& clientId,
    const std::string& tenantId,
    const std::string& redirectUri,
    const std::string& scope
) {
    std::ostringstream url;
    
    url << "https://login.microsoftonline.com/" << tenantId << "/oauth2/v2.0/authorize?"
        << "client_id=" << urlEncode(clientId)
        << "&response_type=code"
        << "&redirect_uri=" << urlEncode(redirectUri)
        << "&scope=" << urlEncode(scope)
        << "&response_mode=query";
    
    LOG_INFO("Built authorization URL");
    
    return url.str();
}

} // namespace Pens

