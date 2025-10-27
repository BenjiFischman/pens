#!/bin/bash

# IMAP Connection Test Script
# Tests IMAP authentication with Hotmail/Outlook

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  IMAP Connection Test for Hotmail/Outlook"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

SERVER="outlook.office365.com"
PORT="993"
USERNAME="memeGodEmporer@hotmail.com"
PASSWORD="Y87p4A4hyuoe*mh2uKfd"

echo "Testing IMAP connection to: $SERVER:$PORT"
echo "Username: $USERNAME"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Test 1: Check server connectivity
echo "📡 Test 1: Checking server connectivity..."
if timeout 5 bash -c "echo > /dev/tcp/$SERVER/$PORT" 2>/dev/null; then
    echo "✅ Server is reachable on port $PORT"
else
    echo "❌ Cannot reach server on port $PORT"
    echo "   Check your internet connection or firewall"
    exit 1
fi
echo ""

# Test 2: Check SSL/TLS connection
echo "🔒 Test 2: Testing SSL/TLS connection..."
echo "   (This will show the SSL handshake)"
echo ""
timeout 5 openssl s_client -connect $SERVER:$PORT -quiet 2>&1 | head -5
echo ""

# Test 3: Get server capabilities
echo "🔍 Test 3: Checking server capabilities..."
echo ""
echo "Sending: A001 CAPABILITY"
echo ""

{
    sleep 1
    echo "A001 CAPABILITY"
    sleep 1
} | openssl s_client -connect $SERVER:$PORT -quiet 2>&1 | grep -A 10 "CAPABILITY" | head -15

echo ""

# Test 4: Try authentication
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔐 Test 4: Testing LOGIN authentication..."
echo ""
echo "Sending: A002 LOGIN \"$USERNAME\" \"********\""
echo ""
echo "Response:"

{
    sleep 1
    echo "A002 LOGIN \"$USERNAME\" \"$PASSWORD\""
    sleep 2
} | openssl s_client -connect $SERVER:$PORT -quiet 2>&1 | grep -A 5 "A002"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📋 Results Interpretation:"
echo ""
echo "✅ 'A002 OK LOGIN completed' = Success! Credentials work!"
echo "❌ 'A002 NO LOGIN failed' = Credentials rejected"
echo "   → Check if IMAP is enabled in Outlook settings"
echo "   → If 2FA enabled, use App Password instead"
echo ""
echo "❌ 'A002 NO [AUTHENTICATIONFAILED]' = Invalid credentials"
echo "   → Password is wrong"
echo "   → Generate new App Password"
echo ""
echo "❌ 'A002 BAD' = Command not supported"
echo "   → IMAP might be disabled"
echo "   → OAuth2 might be required"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "🔧 Next Steps:"
echo ""
echo "1. Enable IMAP:"
echo "   https://outlook.live.com/mail/0/options/mail/accounts"
echo ""
echo "2. Create App Password (if 2FA enabled):"
echo "   https://account.live.com/proofs/AppPassword"
echo ""
echo "3. Check account security:"
echo "   https://account.microsoft.com/security"
echo ""
echo "4. See troubleshooting guide:"
echo "   cat HOTMAIL_TROUBLESHOOTING.md"
echo ""


