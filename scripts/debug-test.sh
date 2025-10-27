#!/bin/bash

# PENS Debug Test Script
# Builds debug version and runs with test credentials

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PENS_DIR="$(dirname "$SCRIPT_DIR")"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  PENS Debug Test Script"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Change to PENS directory
cd "$PENS_DIR"

# Clean and build debug version
echo "📦 Building debug version..."
make clean > /dev/null 2>&1
make debug

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
else
    echo "❌ Build failed!"
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Test Options"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1) Run once and exit (quick test)"
echo "2) Run with LLDB debugger"
echo "3) Run normally (continuous monitoring)"
echo "4) Run with GDB (if available)"
echo "5) Just build (already done)"
echo ""
read -p "Choose option [1-5]: " option

case $option in
    1)
        echo ""
        echo "🔍 Running PENS once in debug mode..."
        echo ""
        ./pens -s outlook.office365.com \
               -u memeGodEmporer@hotmail.com \
               -w "Y87p4A4hyuoe*mh2uKfd" \
               -o \
               -d
        ;;
    2)
        echo ""
        echo "🐛 Starting LLDB debugger..."
        echo ""
        echo "LLDB Commands:"
        echo "  - 'run' or 'r' to start"
        echo "  - 'breakpoint set --file imap_client.cpp --line 131' to set breakpoint"
        echo "  - 'p variable' to print variable"
        echo "  - 'next' or 'n' to step over"
        echo "  - 'step' or 's' to step into"
        echo "  - 'continue' or 'c' to continue"
        echo "  - 'quit' or 'q' to exit"
        echo ""
        lldb ./pens -- \
             -s outlook.office365.com \
             -u memeGodEmporer@hotmail.com \
             -w "Y87p4A4hyuoe*mh2uKfd" \
             -o \
             -d
        ;;
    3)
        echo ""
        echo "🔄 Running PENS in continuous mode..."
        echo "Press Ctrl+C to stop"
        echo ""
        ./pens -s outlook.office365.com \
               -u memeGodEmporer@hotmail.com \
               -w "Y87p4A4hyuoe*mh2uKfd" \
               -d
        ;;
    4)
        if command -v gdb &> /dev/null; then
            echo ""
            echo "🐛 Starting GDB debugger..."
            echo ""
            echo "GDB Commands:"
            echo "  - 'run' or 'r' to start"
            echo "  - 'break imap_client.cpp:131' to set breakpoint"
            echo "  - 'print variable' to print variable"
            echo "  - 'next' or 'n' to step over"
            echo "  - 'step' or 's' to step into"
            echo "  - 'continue' or 'c' to continue"
            echo "  - 'quit' or 'q' to exit"
            echo ""
            gdb --args ./pens \
                -s outlook.office365.com \
                -u memeGodEmporer@hotmail.com \
                -w "Y87p4A4hyuoe*mh2uKfd" \
                -o \
                -d
        else
            echo "❌ GDB not found. Install with: brew install gdb"
            exit 1
        fi
        ;;
    5)
        echo ""
        echo "✅ Build complete! Binary ready at: ./pens"
        ;;
    *)
        echo "Invalid option"
        exit 1
        ;;
esac

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Done!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

