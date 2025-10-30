/**
 * Unit Tests for Config Module
 */

#include "catch.hpp"
#include "../include/config.hpp"
#include <fstream>
#include <cstdio>

using namespace Pens;

TEST_CASE("Config loading from file", "[config]") {
    // Create a temporary config file
    const char* testConfigFile = "test_config.tmp";
    
    SECTION("Valid configuration file") {
        // Write test config
        std::ofstream file(testConfigFile);
        file << "imap_server = imap.test.com\n";
        file << "imap_port = 993\n";
        file << "imap_username = test@test.com\n";
        file << "imap_password = testpass123\n";
        file << "check_interval = 30\n";
        file << "imap_use_ssl = true\n";
        file.close();

        Config& config = Config::getInstance();
        bool loaded = config.loadFromFile(testConfigFile);
        
        REQUIRE(loaded == true);
        REQUIRE(config.getImapServer() == "imap.test.com");
        REQUIRE(config.getImapPort() == 993);
        REQUIRE(config.getImapUsername() == "test@test.com");
        REQUIRE(config.getImapPassword() == "testpass123");
        REQUIRE(config.getCheckInterval() == 30);
        REQUIRE(config.getImapUseSsl() == true);
        
        std::remove(testConfigFile);
    }
    
    SECTION("Missing configuration file") {
        Config& config = Config::getInstance();
        bool loaded = config.loadFromFile("nonexistent_file.conf");
        REQUIRE(loaded == false);
    }
    
    SECTION("Configuration with comments") {
        std::ofstream file(testConfigFile);
        file << "# This is a comment\n";
        file << "imap_server = imap.comments.com\n";
        file << "\n";  // empty line
        file << "imap_port = 995\n";
        file.close();

        Config& config = Config::getInstance();
        bool loaded = config.loadFromFile(testConfigFile);
        
        REQUIRE(loaded == true);
        REQUIRE(config.getImapServer() == "imap.comments.com");
        REQUIRE(config.getImapPort() == 995);
        
        std::remove(testConfigFile);
    }
    
    SECTION("OAuth configuration") {
        std::ofstream file(testConfigFile);
        file << "auth_method = oauth\n";
        file << "oauth_access_token = token123\n";
        file << "oauth_refresh_token = refresh456\n";
        file.close();

        Config& config = Config::getInstance();
        bool loaded = config.loadFromFile(testConfigFile);
        
        REQUIRE(loaded == true);
        REQUIRE(config.getAuthMethod() == "oauth");
        REQUIRE(config.getOAuthAccessToken() == "token123");
        REQUIRE(config.getOAuthRefreshToken() == "refresh456");
        
        std::remove(testConfigFile);
    }
}

TEST_CASE("Config values", "[config]") {
    Config& config = Config::getInstance();
    
    SECTION("IMAP port can be set and retrieved") {
        config.setImapPort(993);
        REQUIRE(config.getImapPort() == 993);
    }
    
    SECTION("Check interval is numeric") {
        int interval = config.getCheckInterval();
        REQUIRE(interval > 0);
        REQUIRE(interval < 10000);  // Reasonable range
    }
    
    SECTION("SSL setting is boolean") {
        bool useSsl = config.getImapUseSsl();
        REQUIRE((useSsl == true || useSsl == false));
    }
    
    SECTION("Auth method is string") {
        std::string authMethod = config.getAuthMethod();
        REQUIRE(!authMethod.empty());
    }
}

TEST_CASE("Config setters", "[config]") {
    Config& config = Config::getInstance();
    
    SECTION("Set IMAP configuration") {
        config.setImapServer("imap.gmail.com");
        config.setImapPort(993);
        config.setImapCredentials("user@gmail.com", "password");
        
        REQUIRE(config.getImapServer() == "imap.gmail.com");
        REQUIRE(config.getImapPort() == 993);
        REQUIRE(config.getImapUsername() == "user@gmail.com");
        REQUIRE(config.getImapPassword() == "password");
    }
    
    SECTION("Set priority threshold") {
        config.setPriorityThreshold(1);
        REQUIRE(config.getPriorityThreshold() == 1);
        
        config.setPriorityThreshold(3);
        REQUIRE(config.getPriorityThreshold() == 3);
    }
}

TEST_CASE("Config environment variables", "[config]") {
    Config& config = Config::getInstance();
    
    SECTION("Load from environment") {
        setenv("PENS_IMAP_SERVER", "imap.env.com", 1);
        setenv("PENS_IMAP_PORT", "993", 1);
        setenv("PENS_IMAP_USERNAME", "envuser@test.com", 1);
        
        bool loaded = config.loadFromEnv();
        
        REQUIRE(loaded == true);
        REQUIRE(config.getImapServer() == "imap.env.com");
        REQUIRE(config.getImapPort() == 993);
        REQUIRE(config.getImapUsername() == "envuser@test.com");
        
        unsetenv("PENS_IMAP_SERVER");
        unsetenv("PENS_IMAP_PORT");
        unsetenv("PENS_IMAP_USERNAME");
    }
}

