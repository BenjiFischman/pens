# Makefile for PENS (Professional Email Notification System)

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -I./include
LDFLAGS = -lssl -lcrypto -lpthread

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Target executable
TARGET = pens

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
HEADERS = $(wildcard $(INC_DIR)/*.hpp)

# Additional object files for new features
VERIFICATION_OBJS = $(OBJ_DIR)/verification_code.o $(OBJ_DIR)/smtp_client.o

# Build modes
DEBUG_FLAGS = -g -O0 -DPENS_DEBUG_BUILD
RELEASE_FLAGS = -O3 -DNDEBUG

# Default target
all: $(TARGET)

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)
	@echo "Debug build complete!"

# Release build
release: CXXFLAGS += $(RELEASE_FLAGS)
release: clean $(TARGET)
	@echo "Release build complete!"

# Create build directories
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Build target
$(TARGET): $(OBJ_DIR) $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✅ Build successful!"
	@echo "PENS is ready for email monitoring"

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Install (copy to /usr/local/bin)
install: release
	@echo "Installing PENS..."
	sudo cp $(TARGET) /usr/local/bin/
	@echo "✅ Installation complete!"
	@echo "   Run 'pens --help' to get started"

# Uninstall
uninstall:
	@echo "Uninstalling PENS..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "✅ Uninstallation complete!"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	rm -f *.o
	rm -f *.log
	rm -f *.gcov *.gcda *.gcno
	rm -f $(TEST_DIR)/*.tmp.log $(TEST_DIR)/*.tmp
	@echo "✅ Clean complete!"

# Clean everything including logs
distclean: clean
	@echo "Deep cleaning..."
	rm -rf logs/
	rm -f pens.log
	@echo "✅ Deep clean complete!"

# Run the application (requires configuration)
run: $(TARGET)
	@echo "Starting PENS..."
	./$(TARGET)

# Test configuration
TEST_DIR = tests
TEST_BUILD_DIR = $(BUILD_DIR)/tests
TEST_TARGET = $(TEST_BUILD_DIR)/pens_tests
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJECTS = $(filter-out $(OBJ_DIR)/main.o, $(OBJECTS))
CATCH_HEADER = $(TEST_DIR)/catch.hpp

# Download Catch2 header if not present
$(CATCH_HEADER):
	@echo "📥 Downloading Catch2 testing framework..."
	@mkdir -p $(TEST_DIR)
	@curl -L -o $(CATCH_HEADER) https://raw.githubusercontent.com/catchorg/Catch2/v2.13.10/single_include/catch2/catch.hpp
	@echo "✅ Catch2 downloaded successfully!"

# Build tests
$(TEST_TARGET): $(CATCH_HEADER) $(TEST_OBJECTS)
	@echo "🧪 Building tests..."
	@mkdir -p $(TEST_BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(TEST_DIR) $(TEST_SOURCES) $(TEST_OBJECTS) -o $(TEST_TARGET) $(LDFLAGS)
	@echo "✅ Tests built successfully!"

# Run tests
test: $(TEST_TARGET)
	@echo "🧪 Running unit tests..."
	@echo ""
	$(TEST_TARGET)
	@echo ""
	@echo "✅ All tests completed!"

# Run tests with verbose output
test-verbose: $(TEST_TARGET)
	@echo "🧪 Running unit tests (verbose)..."
	$(TEST_TARGET) -s

# Run specific test
test-filter: $(TEST_TARGET)
	@echo "🧪 Running filtered tests..."
	$(TEST_TARGET) $(FILTER)

# Run tests and show coverage (requires gcov)
test-coverage: CXXFLAGS += --coverage
test-coverage: LDFLAGS += --coverage
test-coverage: clean $(TEST_TARGET)
	@echo "🧪 Running tests with coverage..."
	$(TEST_TARGET)
	@echo "📊 Generating coverage report..."
	@gcov $(SRC_DIR)/*.cpp
	@echo "✅ Coverage report generated!"

# Docker commands
docker-build:
	@echo "Building Docker image..."
	docker build -t pens:latest .
	@echo "✅ Docker image built!"

docker-run:
	@echo "Running PENS in Docker..."
	docker-compose up -d
	@echo "✅ PENS is running in Docker!"
	@echo "   View logs: docker-compose logs -f"

docker-stop:
	@echo "Stopping PENS Docker container..."
	docker-compose down
	@echo "✅ Container stopped!"

docker-logs:
	docker-compose logs -f pens

docker-shell:
	docker-compose exec pens /bin/bash

# Check dependencies
check-deps:
	@echo "🔍 Checking dependencies..."
	@which $(CXX) > /dev/null || (echo "❌ g++ not found!" && exit 1)
	@pkg-config --exists openssl || (echo "❌ OpenSSL not found!" && exit 1)
	@echo "✅ All dependencies found!"

# Show help
help:
	@echo "PENS (Professional Email Notification System) Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build the application (default)"
	@echo "  debug        - Build with debug symbols"
	@echo "  release      - Build optimized release version"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove all generated files"
	@echo "  run          - Build and run the application"
	@echo "  install      - Install to /usr/local/bin"
	@echo "  uninstall    - Remove from /usr/local/bin"
	@echo "  test         - Run unit tests"
	@echo "  test-verbose - Run tests with verbose output"
	@echo "  test-filter  - Run specific tests (use FILTER='[tag]')"
	@echo "  test-coverage- Run tests with coverage report"
	@echo "  check-deps   - Check if dependencies are installed"
	@echo ""
	@echo "Docker targets:"
	@echo "  docker-build - Build Docker image"
	@echo "  docker-run   - Run in Docker container"
	@echo "  docker-stop  - Stop Docker container"
	@echo "  docker-logs  - View container logs"
	@echo "  docker-shell - Open shell in container"
	@echo ""
	@echo "Example:"
	@echo "  make release          # Build optimized version"
	@echo "  make run              # Build and run"
	@echo "  make docker-build     # Build Docker image"
	@echo ""

# Format code (requires clang-format)
format:
	@echo "🎨 Formatting code..."
	@find $(SRC_DIR) $(INC_DIR) -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
	@echo "✅ Code formatted!"

# Static analysis (requires cppcheck)
analyze:
	@echo "🔍 Running static analysis..."
	@cppcheck --enable=all --std=c++17 --suppress=missingIncludeSystem $(SRC_DIR)
	@echo "✅ Analysis complete!"

# Show project info
info:
	@echo "PENS Project Information"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "Project: Professional Email Notification System"
	@echo "Language: C++17"
	@echo "Compiler: $(CXX)"
	@echo "Source files: $(words $(SOURCES))"
	@echo "Header files: $(words $(HEADERS))"
	@echo "Target: $(TARGET)"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

.PHONY: all debug release clean distclean run test test-verbose test-filter test-coverage \
        install uninstall docker-build docker-run docker-stop docker-logs docker-shell \
        check-deps help format analyze info

