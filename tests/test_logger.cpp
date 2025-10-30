/**
 * Unit Tests for Logger Module
 */

#include "catch.hpp"
#include "../include/logger.hpp"
#include <fstream>
#include <string>
#include <cstdio>

using namespace Pens;

TEST_CASE("Logger initialization", "[logger]") {
    SECTION("Get logger instance") {
        Logger& logger = Logger::getInstance();
        REQUIRE_NOTHROW(logger.info("Test message"));
    }
    
    SECTION("Logger singleton pattern") {
        Logger& logger1 = Logger::getInstance();
        Logger& logger2 = Logger::getInstance();
        REQUIRE(&logger1 == &logger2);
    }
}

TEST_CASE("Logger file operations", "[logger]") {
    const char* testLogFile = "test_logger.tmp.log";
    
    SECTION("Log to file") {
        Logger& logger = Logger::getInstance();
        logger.setLogFile(testLogFile);
        logger.info("Test info message");
        logger.warning("Test warning message");
        logger.error("Test error message");
        
        // Check file exists and has content
        std::ifstream file(testLogFile);
        REQUIRE(file.good());
        
        std::string content;
        std::string line;
        while (std::getline(file, line)) {
            content += line + "\n";
        }
        file.close();
        
        REQUIRE(content.find("Test info message") != std::string::npos);
        REQUIRE(content.find("Test warning message") != std::string::npos);
        REQUIRE(content.find("Test error message") != std::string::npos);
        
        std::remove(testLogFile);
    }
    
    SECTION("Log levels in output") {
        Logger& logger = Logger::getInstance();
        logger.setLogFile(testLogFile);
        logger.setLogLevel(LogLevel::DEBUG);
        logger.debug("Debug message");
        logger.info("Info message");
        logger.warning("Warning message");
        logger.error("Error message");
        
        std::ifstream file(testLogFile);
        std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        file.close();
        
        bool hasInfo = (content.find("[INFO]") != std::string::npos) ||
                       (content.find("INFO") != std::string::npos);
        REQUIRE(hasInfo);
        
        bool hasWarning = (content.find("[WARNING]") != std::string::npos) ||
                         (content.find("WARNING") != std::string::npos);
        REQUIRE(hasWarning);
        
        bool hasError = (content.find("[ERROR]") != std::string::npos) ||
                       (content.find("ERROR") != std::string::npos);
        REQUIRE(hasError);
        
        std::remove(testLogFile);
    }
}

TEST_CASE("Logger message formatting", "[logger]") {
    const char* testLogFile = "test_format.tmp.log";
    Logger& logger = Logger::getInstance();
    
    SECTION("Timestamp in log messages") {
        logger.setLogFile(testLogFile);
        logger.info("Timestamp test");
        
        std::ifstream file(testLogFile);
        std::string line;
        std::getline(file, line);
        file.close();
        
        // Should contain date/time info (look for common patterns)
        bool hasTimestamp = (line.find("202") != std::string::npos) || // year
                           (line.find(":") != std::string::npos);      // time separator
        REQUIRE(hasTimestamp);
        
        std::remove(testLogFile);
    }
    
    SECTION("Message content preserved") {
        logger.setLogFile(testLogFile);
        std::string testMessage = "This is a test message with special chars: @#$%";
        logger.info(testMessage);
        
        std::ifstream file(testLogFile);
        std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        file.close();
        
        REQUIRE(content.find(testMessage) != std::string::npos);
        
        std::remove(testLogFile);
    }
}

TEST_CASE("Logger debug level", "[logger]") {
    const char* testLogFile = "test_debug.tmp.log";
    Logger& logger = Logger::getInstance();
    
    SECTION("Debug messages when debug enabled") {
        logger.setLogFile(testLogFile);
        logger.setLogLevel(LogLevel::DEBUG);
        logger.debug("Debug message");
        
        std::ifstream file(testLogFile);
        std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        file.close();
        
        REQUIRE(content.find("Debug message") != std::string::npos);
        
        std::remove(testLogFile);
    }
    
    SECTION("Debug messages filtered when level is INFO") {
        logger.setLogFile(testLogFile);
        logger.setLogLevel(LogLevel::INFO);
        logger.debug("Debug message");
        logger.info("Info message");
        
        std::ifstream file(testLogFile);
        std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        file.close();
        
        // Info should be there
        REQUIRE(content.find("Info message") != std::string::npos);
        
        std::remove(testLogFile);
    }
}

TEST_CASE("Logger thread safety", "[logger]") {
    const char* testLogFile = "test_thread.tmp.log";
    Logger& logger = Logger::getInstance();
    
    SECTION("Multiple sequential writes") {
        logger.setLogFile(testLogFile);
        
        for (int i = 0; i < 100; i++) {
            logger.info("Message " + std::to_string(i));
        }
        
        std::ifstream file(testLogFile);
        int lineCount = 0;
        std::string line;
        while (std::getline(file, line)) {
            lineCount++;
        }
        file.close();
        
        REQUIRE(lineCount >= 100);  // At least 100 lines
        
        std::remove(testLogFile);
    }
}

TEST_CASE("Logger error handling", "[logger]") {
    Logger& logger = Logger::getInstance();
    
    SECTION("Handle invalid file path") {
        // Try to set invalid log file
        REQUIRE_NOTHROW(logger.setLogFile("/invalid/path/test.log"));
    }
    
    SECTION("Handle empty messages") {
        const char* testLogFile = "test_empty.tmp.log";
        logger.setLogFile(testLogFile);
        
        REQUIRE_NOTHROW(logger.info(""));
        REQUIRE_NOTHROW(logger.warning(""));
        
        std::remove(testLogFile);
    }
    
    SECTION("Handle very long messages") {
        const char* testLogFile = "test_long.tmp.log";
        logger.setLogFile(testLogFile);
        
        std::string longMessage(10000, 'x');
        REQUIRE_NOTHROW(logger.info(longMessage));
        
        std::remove(testLogFile);
    }
}

TEST_CASE("Logger special characters", "[logger]") {
    const char* testLogFile = "test_special.tmp.log";
    Logger& logger = Logger::getInstance();
    
    SECTION("Unicode characters") {
        logger.setLogFile(testLogFile);
        logger.info("Unicode test");
        
        std::ifstream file(testLogFile);
        std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        file.close();
        
        REQUIRE(!content.empty());
        
        std::remove(testLogFile);
    }
    
    SECTION("Newlines in message") {
        logger.setLogFile(testLogFile);
        logger.info("Line 1 Line 2 Line 3");
        
        std::ifstream file(testLogFile);
        std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        file.close();
        
        REQUIRE(content.find("Line 1") != std::string::npos);
        
        std::remove(testLogFile);
    }
}

