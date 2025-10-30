#!/bin/bash

# Setup PENS Testing Environment
# Downloads Catch2 and prepares test infrastructure

set -e

echo "🧪 Setting up PENS testing environment..."
echo ""

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create tests directory
echo "📁 Creating tests directory..."
mkdir -p tests

# Download Catch2 header
echo "📥 Downloading Catch2 testing framework..."
if [ -f "tests/catch.hpp" ]; then
    echo "${YELLOW}⚠️  Catch2 already exists, skipping download${NC}"
else
    curl -L -o tests/catch.hpp https://raw.githubusercontent.com/catchorg/Catch2/v2.13.10/single_include/catch2/catch.hpp
    echo "${GREEN}✅ Catch2 downloaded successfully!${NC}"
fi

# Check dependencies
echo ""
echo "🔍 Checking dependencies..."

# Check for g++
if ! command -v g++ &> /dev/null; then
    echo "❌ g++ not found! Please install:"
    echo "   Ubuntu/Debian: sudo apt-get install g++"
    echo "   macOS: brew install gcc"
    exit 1
fi
echo "${GREEN}✅ g++ found${NC}"

# Check for make
if ! command -v make &> /dev/null; then
    echo "❌ make not found! Please install build-essential"
    exit 1
fi
echo "${GREEN}✅ make found${NC}"

# Check for OpenSSL
if ! pkg-config --exists openssl 2>/dev/null; then
    echo "⚠️  OpenSSL development headers not found"
    echo "   Install with: sudo apt-get install libssl-dev"
fi

# Build tests
echo ""
echo "🔨 Building tests..."
make clean > /dev/null 2>&1 || true
make test

echo ""
echo "${GREEN}════════════════════════════════════════${NC}"
echo "${GREEN}✅ Test environment setup complete!${NC}"
echo "${GREEN}════════════════════════════════════════${NC}"
echo ""
echo "To run tests:"
echo "  make test              # Run all tests"
echo "  make test-verbose      # Verbose output"
echo "  make test-filter FILTER='[config]'  # Specific tests"
echo ""

