# PENS Unit Tests

## 🧪 Test Suite Overview

Comprehensive unit tests for the PENS (Professional Email Notification System) using **Catch2** testing framework.

---

## 📋 Test Coverage

### Test Files

| File | Component | Tests |
|------|-----------|-------|
| `test_config.cpp` | Configuration Management | Config loading, validation, environment variables |
| `test_oauth_helper.cpp` | OAuth Authentication | XOAUTH2, token expiration, base64 encoding |
| `test_verification_code.cpp` | Verification Codes | Generation, validation, expiration |
| `test_logger.cpp` | Logging System | File operations, formatting, thread safety |
| `test_smtp_client.cpp` | SMTP Client | Connection, authentication, email composition |

---

## 🚀 Quick Start

### 1. Install Dependencies

The Catch2 header will be downloaded automatically, but ensure you have build tools:

```bash
# Ubuntu/Debian
sudo apt-get install g++ make curl libssl-dev

# macOS
brew install gcc make curl openssl
```

### 2. Build Tests

```bash
cd /Users/bf/Dev/HelloWorld/Velivolant/pens
make test
```

This will:
1. Download Catch2 header (if not present)
2. Build the test suite
3. Run all tests
4. Display results

---

## 📖 Running Tests

### Run All Tests

```bash
make test
```

**Output:**
```
🧪 Running unit tests...

===============================================================================
All tests passed (247 assertions in 58 test cases)
```

### Run Tests with Verbose Output

```bash
make test-verbose
```

Shows detailed information about each test case.

### Run Specific Tests

```bash
# Run only config tests
make test-filter FILTER="[config]"

# Run only OAuth tests
make test-filter FILTER="[oauth]"

# Run only verification tests
make test-filter FILTER="[verification]"

# Run specific test case
make test-filter FILTER="Config loading from file"
```

### Run with Coverage

```bash
make test-coverage
```

Generates code coverage report using `gcov`.

---

## 🏗️ Test Structure

### Test Organization

```
tests/
├── catch.hpp              # Catch2 framework (auto-downloaded)
├── test_main.cpp         # Test runner
├── test_config.cpp       # Config tests
├── test_oauth_helper.cpp # OAuth tests
├── test_verification_code.cpp # Verification code tests
├── test_logger.cpp       # Logger tests
├── test_smtp_client.cpp  # SMTP client tests
└── README.md             # This file
```

### Test Case Format

```cpp
TEST_CASE("Description", "[tag]") {
    SECTION("Sub-test description") {
        // Arrange
        Config config;
        
        // Act
        bool result = config.loadFromFile("test.conf");
        
        // Assert
        REQUIRE(result == true);
        REQUIRE(config.getImapServer() == "expected.com");
    }
}
```

---

## 📊 Test Categories

### By Tag

Run tests by category:

```bash
# Configuration tests
make test-filter FILTER="[config]"

# OAuth tests
make test-filter FILTER="[oauth]"

# Verification code tests
make test-filter FILTER="[verification]"

# Logger tests
make test-filter FILTER="[logger]"

# SMTP tests
make test-filter FILTER="[smtp]"

# IMAP tests
make test-filter FILTER="[imap]"
```

---

## ✅ Test Assertions

### Common Assertions

```cpp
// Boolean assertions
REQUIRE(expr);              // Must be true
REQUIRE_FALSE(expr);        // Must be false
CHECK(expr);                // Soft assertion (continues on failure)

// Equality
REQUIRE(a == b);
REQUIRE(a != b);

// Comparisons
REQUIRE(a > b);
REQUIRE(a >= b);
REQUIRE(a < b);
REQUIRE(a <= b);

// String operations
REQUIRE(str.find("text") != std::string::npos);
REQUIRE(str.empty());
REQUIRE(!str.empty());

// Exception testing
REQUIRE_THROWS(risky_function());
REQUIRE_NOTHROW(safe_function());
REQUIRE_THROWS_AS(function(), std::exception);
```

---

## 🧩 Writing New Tests

### 1. Create Test File

```cpp
// tests/test_mymodule.cpp
#include "catch.hpp"
#include "../include/mymodule.hpp"

using namespace Pens;

TEST_CASE("My module functionality", "[mymodule]") {
    SECTION("Test basic operation") {
        MyModule module;
        REQUIRE(module.doSomething() == true);
    }
    
    SECTION("Test edge case") {
        MyModule module;
        REQUIRE_THROWS(module.doInvalidOperation());
    }
}
```

### 2. Add to Build

Test files in `tests/` are automatically included in the build.

### 3. Run Tests

```bash
make test
```

---

## 🔍 Debugging Tests

### Debug Build

```bash
# Clean and build with debug symbols
make clean
make debug
make test
```

### Run Under GDB

```bash
# Build tests
make test

# Run under debugger
gdb ./build/tests/pens_tests
```

### Verbose Output

```bash
# See all test output
make test-verbose

# See specific section
make test-filter FILTER="[config]" | grep -A 10 "loading"
```

---

## 📈 Coverage Report

### Generate Coverage

```bash
make test-coverage
```

### View Coverage Files

```
*.gcov         # Coverage data for each source file
```

### Coverage Summary

```bash
# View coverage for specific file
less config.cpp.gcov

# Lines executed are prefixed with execution count
# Lines not executed are marked with #####
```

---

## 🐛 Troubleshooting

### Catch2 Header Not Found

```bash
# Manually download
curl -L -o tests/catch.hpp https://raw.githubusercontent.com/catchorg/Catch2/v2.13.10/single_include/catch2/catch.hpp
```

### Build Failures

```bash
# Clean and rebuild
make clean
make test
```

### Test Failures

```bash
# Run with verbose output
make test-verbose

# Run specific failing test
make test-filter FILTER="failing test name"
```

### Temporary Files Left Behind

```bash
# Clean test artifacts
rm tests/*.tmp tests/*.tmp.log

# Or full clean
make distclean
```

---

## 📚 Resources

### Catch2 Documentation

- [Catch2 Tutorial](https://github.com/catchorg/Catch2/blob/v2.x/docs/tutorial.md)
- [Assertion Macros](https://github.com/catchorg/Catch2/blob/v2.x/docs/assertions.md)
- [Test Cases and Sections](https://github.com/catchorg/Catch2/blob/v2.x/docs/test-cases-and-sections.md)

### Testing Best Practices

1. **Arrange-Act-Assert** pattern
2. **One assertion concept per test**
3. **Use descriptive test names**
4. **Test edge cases and error conditions**
5. **Keep tests independent**
6. **Mock external dependencies**

---

## 🎯 Test Goals

### Current Coverage

- ✅ Configuration loading and validation
- ✅ OAuth helper functions
- ✅ Verification code generation and validation
- ✅ Logger functionality
- ✅ SMTP client commands and formatting

### Future Tests

- ⏳ IMAP client operations
- ⏳ Notification processor
- ⏳ Integration tests with real servers
- ⏳ Performance tests
- ⏳ Stress tests

---

## 🔄 Continuous Integration

### Run Tests in CI

```yaml
# .github/workflows/test.yml
name: PENS Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: sudo apt-get install -y g++ make libssl-dev
      - name: Run tests
        run: make test
```

---

## 📝 Contributing Tests

1. Write tests for new features
2. Ensure all tests pass before committing
3. Add test descriptions to this README
4. Follow existing test patterns
5. Aim for >80% code coverage

---

**Happy Testing!** 🧪✨

