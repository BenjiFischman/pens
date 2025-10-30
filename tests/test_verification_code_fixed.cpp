/**
 * Unit Tests for Verification Code Module
 */

#include "catch.hpp"
#include "../include/verification_code.hpp"
#include <string>
#include <set>

using namespace Pens;

TEST_CASE("Verification code generation", "[verification]") {
    VerificationCodeGenerator generator;
    
    SECTION("Generate 6-digit code") {
        std::string code = generator.generate();
        
        REQUIRE(code.length() == 6);
        
        // Check all characters are digits
        for (char c : code) {
            REQUIRE(isdigit(c));
        }
    }
    
    SECTION("Codes are random") {
        std::set<std::string> codes;
        int numCodes = 50;
        
        for (int i = 0; i < numCodes; i++) {
            codes.insert(generator.generate());
        }
        
        // Should generate many unique codes (allow some duplicates due to randomness)
        REQUIRE(codes.size() > static_cast<size_t>(numCodes / 3));
    }
    
    SECTION("Code range is valid") {
        for (int i = 0; i < 20; i++) {
            std::string code = generator.generate();
            int value = std::stoi(code);
            REQUIRE(value >= 0);
            REQUIRE(value <= 999999);
        }
    }
}

TEST_CASE("Verification code validation", "[verification]") {
    VerificationCodeGenerator generator;
    
    SECTION("Valid 6-digit code") {
        REQUIRE(generator.validate("123456") == true);
        REQUIRE(generator.validate("000000") == true);
        REQUIRE(generator.validate("999999") == true);
    }
    
    SECTION("Invalid - too short") {
        REQUIRE(generator.validate("12345") == false);
        REQUIRE(generator.validate("1") == false);
    }
    
    SECTION("Invalid - too long") {
        REQUIRE(generator.validate("1234567") == false);
        REQUIRE(generator.validate("12345678") == false);
    }
    
    SECTION("Invalid - contains letters") {
        REQUIRE(generator.validate("12AB56") == false);
        REQUIRE(generator.validate("ABCDEF") == false);
    }
    
    SECTION("Invalid - contains special characters") {
        REQUIRE(generator.validate("123-456") == false);
        REQUIRE(generator.validate("123 456") == false);
        REQUIRE(generator.validate("123.456") == false);
    }
    
    SECTION("Invalid - empty string") {
        REQUIRE(generator.validate("") == false);
    }
}

TEST_CASE("Generated codes pass validation", "[verification]") {
    VerificationCodeGenerator generator;
    
    SECTION("All generated codes are valid") {
        for (int i = 0; i < 50; i++) {
            std::string code = generator.generate();
            REQUIRE(generator.validate(code) == true);
        }
    }
}

TEST_CASE("Verification code security", "[verification]") {
    VerificationCodeGenerator generator;
    
    SECTION("Codes are not sequential") {
        std::string code1 = generator.generate();
        std::string code2 = generator.generate();
        
        REQUIRE(code1 != code2);
    }
    
    SECTION("Codes have variation") {
        std::set<std::string> uniqueCodes;
        
        for (int i = 0; i < 100; i++) {
            uniqueCodes.insert(generator.generate());
        }
        
        // Should have many unique codes (>80% unique)
        REQUIRE(uniqueCodes.size() > 80);
    }
}

