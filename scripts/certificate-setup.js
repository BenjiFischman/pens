#!/usr/bin/env node
/**
 * Certificate Setup Helper for PENS OAuth Certificate-Based Authentication
 * ========================================================================
 * 
 * This script generates a self-signed certificate and private key for
 * certificate-based OAuth authentication with Microsoft Azure AD.
 * 
 * Usage:
 *   node scripts/certificate-setup.js
 */

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');
const crypto = require('crypto');
const CERT_DIR = path.join(__dirname, '../certs');
const CERT_FILE = path.join(CERT_DIR, 'pens-cert.pem');
const KEY_FILE = path.join(CERT_DIR, 'pens-key.pem');
const THUMBPRINT_FILE = path.join(CERT_DIR, 'thumbprint.txt');

/**
 * Generate self-signed certificate and private key using OpenSSL
 */
function generateCertificate() {
    console.log('🔐 Generating certificate and private key...\n');
    
    // Create certs directory if it doesn't exist
    if (!fs.existsSync(CERT_DIR)) {
        fs.mkdirSync(CERT_DIR, { recursive: true });
        console.log(`✓ Created directory: ${CERT_DIR}`);
    }
    
    // Check if OpenSSL is available
    try {
        execSync('openssl version', { stdio: 'ignore' });
    } catch (error) {
        throw new Error('OpenSSL is not installed or not in PATH. Please install OpenSSL to generate certificates.');
    }
    
    // Generate private key (2048-bit RSA)
    console.log('  Generating private key...');
    execSync(`openssl genrsa -out "${KEY_FILE}" 2048`, { stdio: 'inherit' });
    
    // Generate self-signed certificate (valid for 1 year)
    console.log('  Generating certificate...');
    const subject = '/C=US/ST=State/L=City/O=PENS/CN=PENS OAuth Certificate';
    execSync(
        `openssl req -new -x509 -key "${KEY_FILE}" -out "${CERT_FILE}" -days 365 -subj "${subject}"`,
        { stdio: 'inherit' }
    );
    
    // Set restrictive permissions (owner read/write only)
    fs.chmodSync(CERT_FILE, 0o600);
    fs.chmodSync(KEY_FILE, 0o600);
    
    console.log(`✓ Certificate saved: ${CERT_FILE}`);
    console.log(`✓ Private key saved: ${KEY_FILE}`);
    console.log('✓ File permissions set to 600 (owner read/write only)\n');
    
    // Read certificate for thumbprint calculation
    const certPEM = fs.readFileSync(CERT_FILE, 'utf8');
    const privateKey = fs.readFileSync(KEY_FILE, 'utf8');
    
    return { certPEM, privateKey };
}

/**
 * Calculate SHA-1 thumbprint of certificate
 * Uses Node.js crypto for cross-platform compatibility
 */
function calculateThumbprint(certPEM) {

    
    // Remove PEM headers and newlines
    const certBase64 = certPEM
        .replace(/-----BEGIN CERTIFICATE-----/g, '')
        .replace(/-----END CERTIFICATE-----/g, '')
        .replace(/\n/g, '')
        .replace(/\r/g, '')
        .trim();
    
    // Decode base64 to get DER format
    const certBuffer = Buffer.from(certBase64, 'base64');
    
    // Calculate SHA-1 hash
    const hash = crypto.createHash('sha1').update(certBuffer).digest();
    
    // Convert to hex string (uppercase, no separators)
    const thumbprintHex = hash.toString('hex').toUpperCase();
    
    // Convert to base64 for Azure AD
    const thumbprintBase64 = hash.toString('base64');
    
    return { hex: thumbprintHex, base64: thumbprintBase64 };
}

/**
 * Format thumbprint with colons (Azure AD format)
 */
function formatThumbprint(thumbprint) {
    return thumbprint.match(/.{2}/g).join(':');
}

/**
 * Display instructions for Azure AD setup
 */
function displayAzureADInstructions(thumbprintHex, thumbprintBase64) {
    console.log('╔════════════════════════════════════════════════════════════════╗');
    console.log('║  Azure AD Certificate Registration Instructions                ║');
    console.log('╚════════════════════════════════════════════════════════════════╝\n');
    
    console.log('1. Go to Azure Portal: https://portal.azure.com');
    console.log('2. Navigate to: Azure Active Directory → App registrations');
    console.log('3. Select your application (or create a new one)');
    console.log('4. Go to "Certificates & secrets" (left sidebar)');
    console.log('5. Click "Upload certificate"');
    console.log('6. Upload the certificate file:');
    console.log(`   ${CERT_FILE}`);
    console.log('7. The thumbprint will be automatically calculated\n');
    
    console.log('📋 Certificate Thumbprint (for reference):');
    console.log(`   Hex format:     ${formatThumbprint(thumbprintHex)}`);
    console.log(`   Base64 format:  ${thumbprintBase64}\n`);
    
    console.log('⚠️  Important Notes:');
    console.log('   - Keep the private key file secure and never share it');
    console.log('   - The certificate is valid for 1 year');
    console.log('   - You can upload multiple certificates to Azure AD');
    console.log('   - After uploading, you can use certificate-based authentication\n');
}

/**
 * Display configuration instructions
 */
function displayConfigInstructions() {
    console.log('╔════════════════════════════════════════════════════════════════╗');
    console.log('║  Configuration Instructions                                    ║');
    console.log('╚════════════════════════════════════════════════════════════════╝\n');
    
    console.log('Add these to your config/pens.conf file:\n');
    console.log('```ini');
    console.log('# OAuth Certificate Authentication');
    console.log('auth_method = oauth');
    console.log(`oauth_certificate_path = ${CERT_FILE}`);
    console.log(`oauth_private_key_path = ${KEY_FILE}`);
    console.log('oauth_client_id = YOUR_AZURE_AD_CLIENT_ID');
    console.log('oauth_tenant_id = common  # or your tenant ID');
    console.log('```\n');
    
    console.log('Or set environment variables:\n');
    console.log('```bash');
    console.log(`export PENS_OAUTH_CERTIFICATE_PATH="${CERT_FILE}"`);
    console.log(`export PENS_OAUTH_PRIVATE_KEY_PATH="${KEY_FILE}"`);
    console.log('export PENS_OAUTH_CLIENT_ID="your-client-id"');
    console.log('export PENS_OAUTH_TENANT_ID="common"');
    console.log('```\n');
}

/**
 * Main function
 */
function main() {
    console.log(`
╔════════════════════════════════════════════════════════════════╗
║                                                                ║
║       PENS Certificate Setup for OAuth Authentication         ║
║                                                                ║
╚════════════════════════════════════════════════════════════════╝
    `);
    
    // Check if certificate already exists
    if (fs.existsSync(CERT_FILE) || fs.existsSync(KEY_FILE)) {
        console.log('⚠️  Certificate files already exist!\n');
        console.log(`   Certificate: ${CERT_FILE}`);
        console.log(`   Private Key: ${KEY_FILE}\n`);
        
        const readline = require('readline');
        const rl = readline.createInterface({
            input: process.stdin,
            output: process.stdout
        });
        
        rl.question('Do you want to overwrite them? (yes/no): ', (answer) => {
            rl.close();
            
            if (answer.toLowerCase() !== 'yes' && answer.toLowerCase() !== 'y') {
                console.log('\n❌ Aborted. Existing files preserved.');
                process.exit(0);
            }
            
            generateAndDisplay();
        });
    } else {
        generateAndDisplay();
    }
}

function generateAndDisplay() {
    try {
        // Generate certificate
        const { certPEM, privateKey } = generateCertificate();
        
        // Calculate thumbprint
        const { hex: thumbprintHex, base64: thumbprintBase64 } = calculateThumbprint(certPEM);
        
        // Save thumbprint to file
        fs.writeFileSync(THUMBPRINT_FILE, `Hex: ${formatThumbprint(thumbprintHex)}\nBase64: ${thumbprintBase64}\n`);
        console.log(`✓ Thumbprint saved: ${THUMBPRINT_FILE}\n`);
        
        // Display instructions
        displayAzureADInstructions(thumbprintHex, thumbprintBase64);
        displayConfigInstructions();
        
        console.log('✅ Certificate setup complete!\n');
        console.log('⚠️  IMPORTANT: Certificate-based auth requires these steps:\n');
        console.log('1. Upload certificate to Azure AD:');
        console.log('   - Go to Azure Portal → App registrations → Your app');
        console.log('   - Certificates & secrets → Upload certificate');
        console.log('   - Upload: ' + CERT_FILE);
        console.log('   - Verify thumbprint matches: ' + formatThumbprint(thumbprintHex) + '\n');
        console.log('2. Rebuild PENS with certificate support:');
        console.log('   cd pens && make clean && make\n');
        console.log('3. Configure certificate paths in config/pens.conf (see above)\n');
        console.log('4. Get initial tokens (still uses device code flow):');
        console.log('   node scripts/oauth-token-helper.js\n');
        console.log('5. After initial tokens, PENS will use certificate for token refresh\n');
        console.log('💡 Run "node scripts/verify-certificate.js" to verify setup\n');
        
    } catch (error) {
        console.error('\n❌ Error:', error.message);
        console.error(error.stack);
        process.exit(1);
    }
}

// Run if executed directly
if (require.main === module) {
    main();
}

module.exports = { generateCertificate, calculateThumbprint };

