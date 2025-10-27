#!/bin/bash

# SENS Quick Start Script
# Builds and runs SENS with interactive setup

set -e

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║   PENS Quick Start - Professional Email Notification System  ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Check if we're in the right directory
if [ ! -f "Makefile" ]; then
    echo "❌ Error: Please run this script from the pens directory"
    exit 1
fi

# Check dependencies
echo "🔍 Checking dependencies..."
if ! command -v g++ &> /dev/null; then
    echo "❌ g++ not found. Please install it first."
    echo "   Run: ./scripts/install-deps.sh"
    exit 1
fi

if ! pkg-config --exists openssl; then
    echo "❌ OpenSSL not found. Please install it first."
    echo "   Run: ./scripts/install-deps.sh"
    exit 1
fi

echo "✅ All dependencies found!"
echo ""

# Build
echo "Building PENS..."
make clean > /dev/null 2>&1 || true
make release

if [ ! -f "pens" ]; then
    echo "❌ Build failed!"
    exit 1
fi

echo "✅ Build successful!"
echo ""

# Interactive configuration
echo "📝 Configuration Setup"
echo "======================"
echo ""

# Check if .env exists
if [ -f ".env" ]; then
    echo "Found existing .env file."
    read -p "Use existing configuration? (y/n): " use_existing
    if [[ $use_existing != "y" && $use_existing != "Y" ]]; then
        configure=true
    else
        configure=false
    fi
else
    configure=true
fi

if [ "$configure" = true ]; then
    echo ""
    echo "Let's set up your email configuration..."
    echo ""
    
    # IMAP Server
    read -p "IMAP Server (default: imap.gmail.com): " imap_server
    imap_server=${imap_server:-imap.gmail.com}
    
    # IMAP Port
    read -p "IMAP Port (default: 993): " imap_port
    imap_port=${imap_port:-993}
    
    # Username
    read -p "Email Username: " username
    
    # Password
    read -s -p "Email Password (or App Password): " password
    echo ""
    
    # Priority Threshold
    echo ""
    read -p "Priority Threshold 1-10 (default: 5): " threshold
    threshold=${threshold:-5}
    
    # Write to .env
    cat > .env << EOF
PENS_IMAP_SERVER=$imap_server
PENS_IMAP_PORT=$imap_port
PENS_IMAP_USERNAME=$username
PENS_IMAP_PASSWORD=$password
PENS_IMAP_USE_SSL=true
PENS_PRIORITY_THRESHOLD=$threshold
PENS_CHECK_INTERVAL=60
PENS_DEBUG_MODE=false
PENS_LOG_LEVEL=INFO
EOF
    
    echo ""
    echo "✅ Configuration saved to .env"
fi

# Load environment variables
export $(cat .env | grep -v '^#' | xargs)

echo ""
echo "Starting PENS..."
echo ""
echo "   Server: $PENS_IMAP_SERVER:$PENS_IMAP_PORT"
echo "   User: $PENS_IMAP_USERNAME"
echo "   Threshold: $PENS_PRIORITY_THRESHOLD/10"
echo ""
echo "Press Ctrl+C to stop"
echo ""
echo "════════════════════════════════════════════════════════════════"
echo ""

# Run PENS
./pens

echo ""
echo "PENS stopped"

