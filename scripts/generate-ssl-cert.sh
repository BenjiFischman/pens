#!/bin/bash
# Generate SSL Certificate for Microsoft Certificate Authorities
# ==============================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CERT_DIR="$SCRIPT_DIR/../certs"
DOMAIN="velivolant.io"
EMAIL="info@velivolant.io"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}"
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║          SSL Certificate Generator for Microsoft          ║"
echo "║                  velivolant.io Domain                     ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Create certs directory
mkdir -p "$CERT_DIR"
cd "$CERT_DIR"

echo -e "${YELLOW}[INFO]${NC} Creating certificates directory: $CERT_DIR"

# Generate private key (2048-bit RSA)
echo -e "\n${BLUE}[1/5]${NC} Generating private key..."
openssl genrsa -out "${DOMAIN}.key" 2048
chmod 600 "${DOMAIN}.key"
echo -e "${GREEN}✓${NC} Private key created: ${DOMAIN}.key"

# Generate Certificate Signing Request (CSR)
echo -e "\n${BLUE}[2/5]${NC} Generating Certificate Signing Request (CSR)..."
openssl req -new -key "${DOMAIN}.key" -out "${DOMAIN}.csr" \
  -subj "/C=US/ST=State/L=City/O=Velivolant/OU=IT/CN=${DOMAIN}/emailAddress=${EMAIL}"
echo -e "${GREEN}✓${NC} CSR created: ${DOMAIN}.csr"

# Create OpenSSL configuration for Subject Alternative Names
cat > "${DOMAIN}.cnf" <<EOF
[req]
default_bits = 2048
prompt = no
default_md = sha256
distinguished_name = dn
req_extensions = v3_req

[dn]
C=US
ST=State
L=City
O=Velivolant
OU=IT Department
emailAddress=${EMAIL}
CN=${DOMAIN}

[v3_req]
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
subjectAltName = @alt_names

[v3_ca]
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always,issuer
basicConstraints = critical, CA:true
keyUsage = critical, digitalSignature, cRLSign, keyCertSign

[alt_names]
DNS.1 = ${DOMAIN}
DNS.2 = *.${DOMAIN}
DNS.3 = mail.${DOMAIN}
DNS.4 = smtp.${DOMAIN}
DNS.5 = imap.${DOMAIN}
DNS.6 = www.${DOMAIN}
email.1 = ${EMAIL}
EOF

# Generate self-signed certificate (for testing)
echo -e "\n${BLUE}[3/5]${NC} Generating self-signed certificate (for testing)..."
openssl req -x509 -new -nodes \
  -key "${DOMAIN}.key" \
  -sha256 \
  -days 365 \
  -out "${DOMAIN}.crt" \
  -config "${DOMAIN}.cnf" \
  -extensions v3_req
echo -e "${GREEN}✓${NC} Self-signed certificate created: ${DOMAIN}.crt"

# Create PKCS#12 bundle (for Windows/Microsoft)
echo -e "\n${BLUE}[4/5]${NC} Creating PKCS#12 bundle for Microsoft..."
openssl pkcs12 -export \
  -out "${DOMAIN}.pfx" \
  -inkey "${DOMAIN}.key" \
  -in "${DOMAIN}.crt" \
  -passout pass:velivolant2025
echo -e "${GREEN}✓${NC} PKCS#12 bundle created: ${DOMAIN}.pfx"
echo -e "${YELLOW}    Password: velivolant2025${NC}"

# Generate CA bundle (if you need to create your own CA)
echo -e "\n${BLUE}[5/5]${NC} Creating Certificate Authority files..."
# Create CA private key
openssl genrsa -out ca.key 4096
chmod 600 ca.key

# Create CA certificate
openssl req -x509 -new -nodes \
  -key ca.key \
  -sha256 \
  -days 3650 \
  -out ca.crt \
  -subj "/C=US/ST=State/L=City/O=Velivolant/OU=Certificate Authority/CN=Velivolant Root CA/emailAddress=${EMAIL}" \
  -extensions v3_ca \
  -config "${DOMAIN}.cnf"

echo -e "${GREEN}✓${NC} CA certificate created: ca.crt"

# Sign the domain certificate with CA
echo -e "\n${BLUE}[BONUS]${NC} Creating CA-signed certificate..."
openssl x509 -req \
  -in "${DOMAIN}.csr" \
  -CA ca.crt \
  -CAkey ca.key \
  -CAcreateserial \
  -out "${DOMAIN}-ca-signed.crt" \
  -days 365 \
  -sha256 \
  -extfile "${DOMAIN}.cnf" \
  -extensions v3_req

echo -e "${GREEN}✓${NC} CA-signed certificate created: ${DOMAIN}-ca-signed.crt"

# Create full chain certificate
cat "${DOMAIN}-ca-signed.crt" ca.crt > "${DOMAIN}-fullchain.crt"
echo -e "${GREEN}✓${NC} Full chain certificate created: ${DOMAIN}-fullchain.crt"

# Display certificate information
echo -e "\n${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✓ Certificate Generation Complete!${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"

echo -e "\n${YELLOW}📁 Generated Files:${NC}"
echo ""
echo "  Private Keys:"
echo "  • ${DOMAIN}.key            - Domain private key (KEEP SECURE!)"
echo "  • ca.key                   - CA private key (KEEP SECURE!)"
echo ""
echo "  Certificates:"
echo "  • ${DOMAIN}.crt            - Self-signed certificate"
echo "  • ${DOMAIN}-ca-signed.crt  - CA-signed certificate"
echo "  • ${DOMAIN}-fullchain.crt  - Full certificate chain"
echo "  • ca.crt                   - Certificate Authority certificate"
echo ""
echo "  For Microsoft:"
echo "  • ${DOMAIN}.pfx            - PKCS#12 bundle (password: velivolant2025)"
echo ""
echo "  Request Files:"
echo "  • ${DOMAIN}.csr            - Certificate Signing Request"
echo "  • ${DOMAIN}.cnf            - OpenSSL configuration"
echo ""

echo -e "${YELLOW}📋 Certificate Information:${NC}"
openssl x509 -in "${DOMAIN}.crt" -text -noout | grep -A 2 "Subject:"
openssl x509 -in "${DOMAIN}.crt" -text -noout | grep -A 1 "Subject Alternative Name"
openssl x509 -in "${DOMAIN}.crt" -text -noout | grep "Not After"

echo -e "\n${YELLOW}🔐 Security Notes:${NC}"
echo "  • Keep *.key files secure and NEVER commit to git"
echo "  • Password for .pfx file: velivolant2025"
echo "  • These are self-signed certificates for testing"
echo "  • For production, get certificates from a trusted CA"

echo -e "\n${BLUE}📤 Next Steps:${NC}"
echo ""
echo "For Microsoft 365 / Azure AD:"
echo "  1. Use ${DOMAIN}.pfx for uploading to Azure AD"
echo "  2. Use ca.crt to add your CA to trusted roots"
echo ""
echo "For production SSL/TLS:"
echo "  1. Use ${DOMAIN}.csr to request from a real CA (Let's Encrypt, DigiCert, etc.)"
echo "  2. Install the CA-issued certificate on your servers"
echo ""
echo "For S/MIME email encryption:"
echo "  1. Use ${DOMAIN}.pfx to configure Outlook S/MIME"
echo "  2. Share ca.crt with recipients for verification"
echo ""

echo -e "${GREEN}Done!${NC}\n"

# Set proper permissions
chmod 644 *.crt *.csr *.cnf *.pfx ca.crt.srl 2>/dev/null || true
chmod 600 *.key 2>/dev/null || true

echo -e "${YELLOW}All files saved to: ${CERT_DIR}${NC}\n"


# Create .cer files (Windows format, same as .crt)
echo -e "\n${BLUE}[BONUS]${NC} Creating Windows-compatible .cer files..."
cp "${DOMAIN}.crt" "${DOMAIN}.cer"
cp "${DOMAIN}-ca-signed.crt" "${DOMAIN}-ca-signed.cer"
cp "ca.crt" "ca.cer"
echo -e "${GREEN}✓${NC} Windows .cer files created"
