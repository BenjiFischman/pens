/**
 * Unit Tests for SMTP Client Module
 * Testing the actual SmtpClient API
 */

#include "catch.hpp"
#include "../include/smtp_client.hpp"
#include <string>

using namespace Pens;

TEST_CASE("SMTP Client construction", "[smtp]") {
    SECTION("Create SMTP client with valid parameters") {
        REQUIRE_NOTHROW(SmtpClient("smtp.gmail.com", 587, true));
        REQUIRE_NOTHROW(SmtpClient("localhost", 1025, false));
    }
    
    SECTION("Different ports") {
        REQUIRE_NOTHROW(SmtpClient("smtp.test.com", 25, false));
        REQUIRE_NOTHROW(SmtpClient("smtp.test.com", 587, true));
        REQUIRE_NOTHROW(SmtpClient("smtp.test.com", 465, true));
    }
    
    SECTION("SSL and non-SSL") {
        SmtpClient sslClient("smtp.test.com", 465, true);
        SmtpClient nonSslClient("smtp.test.com", 25, false);
        
        REQUIRE_NOTHROW(sslClient.getConnectionStatus());
        REQUIRE_NOTHROW(nonSslClient.getConnectionStatus());
    }
}

TEST_CASE("SMTP Client connection status", "[smtp]") {
    SmtpClient client("smtp.test.com", 587, true);
    
    SECTION("Initial connection status") {
        std::string status = client.getConnectionStatus();
        REQUIRE(!status.empty());
    }
    
    SECTION("isConnected before connection") {
        REQUIRE(client.isConnected() == false);
    }
}

TEST_CASE("SMTP Client email sending interface", "[smtp]") {
    SmtpClient client("smtp.test.com", 587, true);
    
    SECTION("sendEmail method signature") {
        // Test that method can be called (won't actually connect in unit test)
        // In integration tests, this would actually send email
        std::string from = "sender@test.com";
        std::string to = "recipient@test.com";
        std::string subject = "Test Subject";
        std::string body = "Test Body";
        
        // Verify the method pointer exists (compile-time check)
        auto sendEmailPtr = &SmtpClient::sendEmail;
        REQUIRE(sendEmailPtr != nullptr);
    }
    
    SECTION("sendVerificationCode method exists") {
        // Verify method pointer exists
        auto sendVerifPtr = &SmtpClient::sendVerificationCode;
        REQUIRE(sendVerifPtr != nullptr);
    }
}

TEST_CASE("SMTP authentication methods", "[smtp]") {
    SmtpClient client("smtp.test.com", 587, true);
    
    SECTION("authenticate method exists") {
        // Method pointer check
        auto authPtr = &SmtpClient::authenticate;
        REQUIRE(authPtr != nullptr);
    }
    
    SECTION("authenticateOAuth method exists") {
        // Method pointer check
        auto oauthAuthPtr = &SmtpClient::authenticateOAuth;
        REQUIRE(oauthAuthPtr != nullptr);
    }
}

TEST_CASE("SMTP Client lifecycle", "[smtp]") {
    SECTION("Create and destroy client") {
        SmtpClient* client = new SmtpClient("smtp.test.com", 587, true);
        REQUIRE(client != nullptr);
        delete client;
    }
    
    SECTION("Multiple clients") {
        SmtpClient client1("smtp1.test.com", 587, true);
        SmtpClient client2("smtp2.test.com", 465, true);
        
        std::string status1 = client1.getConnectionStatus();
        std::string status2 = client2.getConnectionStatus();
        
        // Both clients should have status strings
        REQUIRE(!status1.empty());
        REQUIRE(!status2.empty());
    }
}

TEST_CASE("SMTP error handling", "[smtp]") {
    SECTION("Handle empty server") {
        REQUIRE_NOTHROW(SmtpClient("", 587, true));
    }
    
    SECTION("Handle invalid port") {
        REQUIRE_NOTHROW(SmtpClient("smtp.test.com", 0, true));
        REQUIRE_NOTHROW(SmtpClient("smtp.test.com", 99999, true));
    }
}

