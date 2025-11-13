#include "oauth_helper.hpp"
#include "logger.hpp"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <fstream>
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

// Base64 URL encoding (for JWT)
std::string OAuthHelper::base64UrlEncode(const std::string& input) {
    std::string encoded = base64Encode(input);
    
    // Convert base64 to base64url: replace + with -, / with _, remove =
    std::replace(encoded.begin(), encoded.end(), '+', '-');
    std::replace(encoded.begin(), encoded.end(), '/', '_');
    encoded.erase(std::remove(encoded.begin(), encoded.end(), '='), encoded.end());
    
    return encoded;
}

std::string OAuthHelper::calculateCertificateThumbprint(const std::string& certificatePath) {
    FILE* certFile = fopen(certificatePath.c_str(), "r");
    if (!certFile) {
        LOG_ERROR("Failed to open certificate file: " + certificatePath);
        return "";
    }
    
    X509* cert = PEM_read_X509(certFile, nullptr, nullptr, nullptr);
    fclose(certFile);
    
    if (!cert) {
        LOG_ERROR("Failed to parse certificate: " + certificatePath);
        return "";
    }
    
    // Get certificate DER encoding
    unsigned char* der = nullptr;
    int derLen = i2d_X509(cert, &der);
    
    if (derLen < 0) {
        LOG_ERROR("Failed to encode certificate to DER");
        X509_free(cert);
        return "";
    }
    
    // Calculate SHA-1 hash
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(der, derLen, hash);
    OPENSSL_free(der);
    X509_free(cert);
    
    // Convert to base64url (Azure AD x5t requires base64url encoding)
    std::string hashStr(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
    return base64UrlEncode(hashStr);
}

std::string OAuthHelper::generateClientAssertion(
    const std::string& clientId,
    const std::string& tenantId,
    const std::string& certificatePath,
    const std::string& privateKeyPath
) {
    // Load private key
    FILE* keyFile = fopen(privateKeyPath.c_str(), "r");
    if (!keyFile) {
        LOG_ERROR("Failed to open private key file: " + privateKeyPath);
        return "";
    }
    
    EVP_PKEY* privateKey = PEM_read_PrivateKey(keyFile, nullptr, nullptr, nullptr);
    fclose(keyFile);
    
    if (!privateKey) {
        LOG_ERROR("Failed to load private key: " + privateKeyPath);
        return "";
    }
    
    // Load certificate to get thumbprint
    std::string thumbprint = calculateCertificateThumbprint(certificatePath);
    if (thumbprint.empty()) {
        LOG_ERROR("Failed to calculate certificate thumbprint");
        EVP_PKEY_free(privateKey);
        return "";
    }
    
    LOG_DEBUG("Certificate thumbprint (base64url): " + thumbprint);
    
    // Build JWT header
    std::ostringstream headerJson;
    headerJson << R"({"alg":"RS256","typ":"JWT","x5t":")" << thumbprint << R"("})";
    std::string header = base64UrlEncode(headerJson.str());
    
    // Build JWT payload
    long now = std::time(nullptr);
    long exp = now + 3600; // 1 hour expiration
    
    // Generate unique JWT ID (jti) using timestamp and random component
    std::ostringstream jtiStream;
    jtiStream << std::hex << now << "-" << (now % 1000000);
    std::string jti = jtiStream.str();
    
    std::ostringstream payloadJson;
    payloadJson << R"({"aud":"https://login.microsoftonline.com/)" << tenantId
                << R"(/oauth2/v2.0/token","exp":)" << exp
                << R"(,"iss":")" << clientId
                << R"(","jti":")" << jti
                << R"(","nbf":)" << now
                << R"(,"sub":")" << clientId << R"("})";
    std::string payload = base64UrlEncode(payloadJson.str());
    
    // Create signature input
    std::string signatureInput = header + "." + payload;
    
    // Sign with RSA-SHA256
    EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
    if (!mdCtx) {
        LOG_ERROR("Failed to create EVP_MD_CTX");
        EVP_PKEY_free(privateKey);
        return "";
    }
    
    if (EVP_DigestSignInit(mdCtx, nullptr, EVP_sha256(), nullptr, privateKey) != 1) {
        LOG_ERROR("Failed to initialize signature");
        EVP_MD_CTX_free(mdCtx);
        EVP_PKEY_free(privateKey);
        return "";
    }
    
    if (EVP_DigestSignUpdate(mdCtx, signatureInput.c_str(), signatureInput.length()) != 1) {
        LOG_ERROR("Failed to update signature");
        EVP_MD_CTX_free(mdCtx);
        EVP_PKEY_free(privateKey);
        return "";
    }
    
    size_t sigLen = 0;
    if (EVP_DigestSignFinal(mdCtx, nullptr, &sigLen) != 1) {
        LOG_ERROR("Failed to get signature length");
        EVP_MD_CTX_free(mdCtx);
        EVP_PKEY_free(privateKey);
        return "";
    }
    
    unsigned char* signature = new unsigned char[sigLen];
    if (EVP_DigestSignFinal(mdCtx, signature, &sigLen) != 1) {
        LOG_ERROR("Failed to finalize signature");
        delete[] signature;
        EVP_MD_CTX_free(mdCtx);
        EVP_PKEY_free(privateKey);
        return "";
    }
    
    EVP_MD_CTX_free(mdCtx);
    EVP_PKEY_free(privateKey);
    
    // Encode signature
    std::string sigStr(reinterpret_cast<char*>(signature), sigLen);
    std::string signatureEncoded = base64UrlEncode(sigStr);
    delete[] signature;
    
    // Build final JWT
    std::string jwt = header + "." + payload + "." + signatureEncoded;
    
    LOG_INFO("Generated client assertion JWT");
    return jwt;
}

} // namespace Pens

