#!/usr/bin/env node
/**
 * Verify that certificate and private key match
 */

const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

const CERT_FILE = process.argv[2] || path.join(__dirname, '../certs/pens-cert.pem');
const KEY_FILE = process.argv[3] || path.join(__dirname, '../certs/pens-key.pem');

function verifyMatch() {
  console.log('🔍 Verifying certificate and private key match...\n');
  
  if (!fs.existsSync(CERT_FILE)) {
    console.error(`❌ Certificate file not found: ${CERT_FILE}`);
    process.exit(1);
  }
  
  if (!fs.existsSync(KEY_FILE)) {
    console.error(`❌ Private key file not found: ${KEY_FILE}`);
    process.exit(1);
  }
  
  try {
    // Read certificate
    const certPEM = fs.readFileSync(CERT_FILE, 'utf8');
    const cert = crypto.createPublicKey(certPEM);
    
    // Read private key
    const keyPEM = fs.readFileSync(KEY_FILE, 'utf8');
    const privateKey = crypto.createPrivateKey(keyPEM);
    
    // Extract public key from private key
    const publicKeyFromPrivate = crypto.createPublicKey(privateKey);
    
    // Compare public keys
    const certPublicKey = cert.export({ type: 'spki', format: 'pem' });
    const privatePublicKey = publicKeyFromPrivate.export({ type: 'spki', format: 'pem' });
    
    if (certPublicKey === privatePublicKey) {
      console.log('✅ Certificate and private key MATCH!\n');
      
      // Test signature
      const testData = 'test signature data';
      const sign = crypto.createSign('RSA-SHA256');
      sign.update(testData);
      sign.end();
      const signature = sign.sign(privateKey);
      
      const verify = crypto.createVerify('RSA-SHA256');
      verify.update(testData);
      verify.end();
      const isValid = verify.verify(cert, signature);
      
      if (isValid) {
        console.log('✅ Signature verification test PASSED!\n');
        console.log('The certificate and private key are correctly paired.');
        return true;
      } else {
        console.error('❌ Signature verification test FAILED!');
        console.error('The keys match but signature verification failed.');
        return false;
      }
    } else {
      console.error('❌ Certificate and private key DO NOT MATCH!\n');
      console.error('The certificate and private key are from different key pairs.');
      console.error('Solution: Regenerate the certificate and key pair:');
      console.error('  node scripts/certificate-setup.js\n');
      return false;
    }
  } catch (error) {
    console.error(`❌ Error: ${error.message}`);
    if (error.message.includes('PEM')) {
      console.error('\nThe certificate or private key file is not in valid PEM format.');
    }
    return false;
  }
}

if (require.main === module) {
  const success = verifyMatch();
  process.exit(success ? 0 : 1);
}

module.exports = { verifyMatch };

