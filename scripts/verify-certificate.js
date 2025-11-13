#!/usr/bin/env node
/**
 * Certificate Verification Script for PENS
 * 
 * Verifies that certificate files exist and can be read,
 * and displays the thumbprint for Azure AD registration.
 */

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const CERT_DIR = path.join(__dirname, '../certs');
const CERT_FILE = path.join(CERT_DIR, 'pens-cert.pem');
const KEY_FILE = path.join(CERT_DIR, 'pens-key.pem');

function calculateThumbprint(certPEM) {
    const certBase64 = certPEM
        .replace(/-----BEGIN CERTIFICATE-----/g, '')
        .replace(/-----END CERTIFICATE-----/g, '')
        .replace(/\n/g, '')
        .replace(/\r/g, '')
        .trim();
    
    const certBuffer = Buffer.from(certBase64, 'base64');
    const hash = crypto.createHash('sha1').update(certBuffer).digest();
    const thumbprintHex = hash.toString('hex').toUpperCase();
    const thumbprintBase64 = hash.toString('base64');
    
    return { hex: thumbprintHex, base64: thumbprintBase64 };
}

function formatThumbprint(thumbprint) {
    return thumbprint.match(/.{2}/g).join(':');
}

function verifyCertificate() {
    console.log('🔍 Verifying certificate setup...\n');
    
    // Check if files exist
    if (!fs.existsSync(CERT_FILE)) {
        console.error(`❌ Certificate file not found: ${CERT_FILE}`);
        console.error('   Run: node scripts/certificate-setup.js');
        return false;
    }
    
    if (!fs.existsSync(KEY_FILE)) {
        console.error(`❌ Private key file not found: ${KEY_FILE}`);
        console.error('   Run: node scripts/certificate-setup.js');
        return false;
    }
    
    console.log(`✓ Certificate file exists: ${CERT_FILE}`);
    console.log(`✓ Private key file exists: ${KEY_FILE}\n`);
    
    // Check file permissions
    const certStats = fs.statSync(CERT_FILE);
    const keyStats = fs.statSync(KEY_FILE);
    const certMode = (certStats.mode & parseInt('777', 8)).toString(8);
    const keyMode = (keyStats.mode & parseInt('777', 8)).toString(8);
    
    console.log(`  Certificate permissions: ${certMode}`);
    console.log(`  Private key permissions: ${keyMode}`);
    
    if (certMode !== '600' && certMode !== '400') {
        console.warn('  ⚠️  Certificate should be 600 (read/write owner only)');
    }
    if (keyMode !== '600' && keyMode !== '400') {
        console.warn('  ⚠️  Private key should be 600 (read/write owner only)');
    }
    console.log('');
    
    // Try to read and parse certificate
    try {
        const certPEM = fs.readFileSync(CERT_FILE, 'utf8');
        
        if (!certPEM.includes('BEGIN CERTIFICATE')) {
            console.error('❌ Certificate file does not appear to be a valid PEM certificate');
            return false;
        }
        
        console.log('✓ Certificate file is valid PEM format');
        
        // Calculate thumbprint
        const { hex, base64 } = calculateThumbprint(certPEM);
        
        console.log('\n📋 Certificate Thumbprint:');
        console.log(`   Hex format:     ${formatThumbprint(hex)}`);
        console.log(`   Base64 format:  ${base64}`);
        console.log('\n💡 Make sure this thumbprint matches what\'s registered in Azure AD!');
        console.log('   Azure Portal → App registrations → Your app → Certificates & secrets\n');
        
    } catch (error) {
        console.error(`❌ Failed to read certificate: ${error.message}`);
        return false;
    }
    
    // Try to read private key
    try {
        const keyPEM = fs.readFileSync(KEY_FILE, 'utf8');
        
        if (!keyPEM.includes('BEGIN') || !keyPEM.includes('PRIVATE KEY')) {
            console.error('❌ Private key file does not appear to be a valid PEM private key');
            return false;
        }
        
        console.log('✓ Private key file is valid PEM format\n');
        
    } catch (error) {
        console.error(`❌ Failed to read private key: ${error.message}`);
        return false;
    }
    
    // Check config file
    const configPath = path.join(__dirname, '../config/pens.conf');
    if (fs.existsSync(configPath)) {
        const configContent = fs.readFileSync(configPath, 'utf8');
        const hasCertPath = configContent.includes('oauth_certificate_path');
        const hasKeyPath = configContent.includes('oauth_private_key_path');
        
        console.log('📝 Configuration file check:');
        if (hasCertPath && hasKeyPath) {
            console.log('✓ Certificate paths found in config file');
        } else {
            console.warn('⚠️  Certificate paths not found in config file');
            console.warn('   Add these lines to config/pens.conf:');
            console.warn(`   oauth_certificate_path = ${CERT_FILE}`);
            console.warn(`   oauth_private_key_path = ${KEY_FILE}`);
        }
        console.log('');
    }
    
    console.log('✅ Certificate verification complete!\n');
    console.log('Next steps:');
    console.log('1. Upload the certificate to Azure AD (if not already done)');
    console.log('2. Verify the thumbprint matches what\'s in Azure AD');
    console.log('3. Ensure certificate paths are in config/pens.conf');
    console.log('4. Test token refresh with PENS\n');
    
    return true;
}

if (require.main === module) {
    const success = verifyCertificate();
    process.exit(success ? 0 : 1);
}

module.exports = { verifyCertificate, calculateThumbprint };

