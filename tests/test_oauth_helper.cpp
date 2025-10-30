/**
 * Unit Tests for OAuth Helper Module
 */

#include "catch.hpp"
#include "../include/oauth_helper.hpp"
#include <string>

using namespace Pens;

TEST_CASE("OAuth XOAUTH2 string generation", "[oauth]") {
    SECTION("Generate valid XOAUTH2 string") {
        std::string email = "test@example.com";
        std::string accessToken = "ya29.test_token_12345";
        
        std::string xoauth2 = OAuthHelper::generateXOAuth2String(email, accessToken);
        
        // Should not be empty
        REQUIRE(!xoauth2.empty());
        REQUIRE(xoauth2.length() > 0);
        
        // Should be base64 encoded (only contains valid base64 characters)
        bool isBase64 = true;
        for (char c : xoauth2) {
            if (!isalnum(c) && c != '+' && c != '/' && c != '=') {
                isBase64 = false;
                break;
            }
        }
        REQUIRE(isBase64 == true);
    }
    
    SECTION("Empty email") {
        std::string email = "";
        std::string accessToken = "token123";
        
        std::string xoauth2 = OAuthHelper::generateXOAuth2String(email, accessToken);
        
        // Should still generate something (for testing invalid scenarios)
        REQUIRE(!xoauth2.empty());
    }
    
    SECTION("Empty token") {
        std::string email = "test@example.com";
        std::string accessToken = "";
        
        std::string xoauth2 = OAuthHelper::generateXOAuth2String(email, accessToken);
        
        // Should still generate something
        REQUIRE(!xoauth2.empty());
    }
    
    SECTION("Special characters in email") {
        std::string email = "test+tag@example.com";
        std::string accessToken = "token123";
        
        REQUIRE_NOTHROW(OAuthHelper::generateXOAuth2String(email, accessToken));
    }
    
    SECTION("Different emails generate different strings") {
        std::string email1 = "user1@test.com";
        std::string email2 = "user2@test.com";
        std::string token = "same_token";
        
        std::string xoauth1 = OAuthHelper::generateXOAuth2String(email1, token);
        std::string xoauth2 = OAuthHelper::generateXOAuth2String(email2, token);
        
        REQUIRE(xoauth1 != xoauth2);
    }
}

TEST_CASE("OAuth token expiration check", "[oauth]") {
    SECTION("Token not expired") {
        time_t now = time(nullptr);
        time_t tokenTimestamp = now - 1800;  // 30 minutes ago
        int expiresIn = 3600;  // 1 hour lifetime
        
        bool expired = OAuthHelper::isTokenExpired(tokenTimestamp, expiresIn);
        REQUIRE(expired == false);
    }
    
    SECTION("Token expired") {
        time_t now = time(nullptr);
        time_t tokenTimestamp = now - 7200;  // 2 hours ago
        int expiresIn = 3600;  // 1 hour lifetime
        
        bool expired = OAuthHelper::isTokenExpired(tokenTimestamp, expiresIn);
        REQUIRE(expired == true);
    }
    
    SECTION("Token just expired") {
        time_t now = time(nullptr);
        time_t tokenTimestamp = now - 3601;  // 1 hour and 1 second ago
        int expiresIn = 3600;  // 1 hour lifetime
        
        bool expired = OAuthHelper::isTokenExpired(tokenTimestamp, expiresIn);
        REQUIRE(expired == true);
    }
    
    SECTION("Fresh token") {
        time_t now = time(nullptr);
        time_t tokenTimestamp = now - 60;  // 1 minute ago
        int expiresIn = 3600;  // 1 hour lifetime
        
        bool expired = OAuthHelper::isTokenExpired(tokenTimestamp, expiresIn);
        REQUIRE(expired == false);
    }
    
    SECTION("Token just acquired") {
        time_t now = time(nullptr);
        time_t tokenTimestamp = now;  // Right now
        int expiresIn = 3600;
        
        bool expired = OAuthHelper::isTokenExpired(tokenTimestamp, expiresIn);
        REQUIRE(expired == false);
    }
}

TEST_CASE("OAuth authorization URL building", "[oauth]") {
    SECTION("Build authorization URL") {
        std::string clientId = "client123";
        std::string tenantId = "common";
        std::string redirectUri = "http://localhost/callback";
        std::string scope = "https://outlook.office365.com/.default";
        
        std::string url = OAuthHelper::buildAuthorizationUrl(
            clientId, tenantId, redirectUri, scope
        );
        
        REQUIRE(!url.empty());
        REQUIRE(url.find("login.microsoftonline.com") != std::string::npos);
        REQUIRE(url.find(clientId) != std::string::npos);
        REQUIRE(url.find("oauth2/v2.0/authorize") != std::string::npos);
    }
    
    SECTION("URL contains required parameters") {
        std::string url = OAuthHelper::buildAuthorizationUrl(
            "client_id_test",
            "common",
            "http://localhost/callback",
            "Mail.Read"
        );
        
        REQUIRE(url.find("client_id") != std::string::npos);
        REQUIRE(url.find("redirect_uri") != std::string::npos);
        REQUIRE(url.find("scope") != std::string::npos);
        REQUIRE(url.find("response_type") != std::string::npos);
    }
    
    SECTION("Different tenants") {
        std::string url1 = OAuthHelper::buildAuthorizationUrl(
            "client123", "common", "http://localhost/callback", "Mail.Read"
        );
        std::string url2 = OAuthHelper::buildAuthorizationUrl(
            "client123", "consumers", "http://localhost/callback", "Mail.Read"
        );
        
        REQUIRE(url1.find("common") != std::string::npos);
        REQUIRE(url2.find("consumers") != std::string::npos);
        REQUIRE(url1 != url2);
    }
}

TEST_CASE("OAuth error handling", "[oauth]") {
    SECTION("Handle empty values gracefully") {
        REQUIRE_NOTHROW(OAuthHelper::generateXOAuth2String("", ""));
        REQUIRE_NOTHROW(OAuthHelper::buildAuthorizationUrl("", "", "", ""));
    }
    
    SECTION("Large token doesn't cause issues") {
        std::string largeToken(10000, 'x');  // 10KB token
        std::string email = "test@example.com";
        
        std::string result;
        REQUIRE_NOTHROW(result = OAuthHelper::generateXOAuth2String(email, largeToken));
        REQUIRE(!result.empty());
    }
    
    SECTION("Special characters in parameters") {
        std::string email = "test+tag@example.com";
        std::string token = "token/with+special=chars";
        
        REQUIRE_NOTHROW(OAuthHelper::generateXOAuth2String(email, token));
    }
}

TEST_CASE("OAuth consistency", "[oauth]") {
    SECTION("Same input produces same output") {
        std::string email = "test@example.com";
        std::string token = "token123";
        
        std::string result1 = OAuthHelper::generateXOAuth2String(email, token);
        std::string result2 = OAuthHelper::generateXOAuth2String(email, token);
        
        REQUIRE(result1 == result2);
    }
    
    SECTION("Authorization URLs are deterministic") {
        std::string url1 = OAuthHelper::buildAuthorizationUrl(
            "client", "tenant", "redirect", "scope"
        );
        std::string url2 = OAuthHelper::buildAuthorizationUrl(
            "client", "tenant", "redirect", "scope"
        );
        
        REQUIRE(url1 == url2);
    }
}

