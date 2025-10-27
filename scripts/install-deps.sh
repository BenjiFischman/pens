#!/bin/bash

# SENS Dependency Installation Script
# Supports Ubuntu/Debian and macOS

set -e

echo "PENS Dependency Installer"
echo "=============================="

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "📦 Detected Linux (Ubuntu/Debian)"
    
    # Update package list
    echo "Updating package list..."
    sudo apt-get update
    
    # Install dependencies
    echo "Installing dependencies..."
    sudo apt-get install -y \
        build-essential \
        g++ \
        make \
        libssl-dev \
        pkg-config \
        git
    
    echo "✅ Dependencies installed successfully!"
    
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "📦 Detected macOS"
    
    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        echo "❌ Homebrew not found. Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    # Install dependencies
    echo "Installing dependencies..."
    brew install openssl gcc make pkg-config
    
    echo "✅ Dependencies installed successfully!"
    
else
    echo "❌ Unsupported operating system: $OSTYPE"
    exit 1
fi

# Verify installation
echo ""
echo "🔍 Verifying installation..."

if command -v g++ &> /dev/null; then
    echo "✅ g++ found: $(g++ --version | head -n 1)"
else
    echo "❌ g++ not found"
    exit 1
fi

if command -v make &> /dev/null; then
    echo "✅ make found: $(make --version | head -n 1)"
else
    echo "❌ make not found"
    exit 1
fi

if pkg-config --exists openssl; then
    echo "✅ OpenSSL found: $(pkg-config --modversion openssl)"
else
    echo "❌ OpenSSL not found"
    exit 1
fi

echo ""
echo "✅ All dependencies are installed!"
echo ""
echo "Next steps:"
echo "  1. Build PENS: make release"
echo "  2. Run PENS: ./pens --help"
echo ""

