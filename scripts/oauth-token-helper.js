#!/usr/bin/env node
/**
 * OAuth 2.0 Token Helper for Microsoft 365
 * ========================================
 * 
 * This script helps you get OAuth 2.0 access tokens for PENS
 * to authenticate with Microsoft 365 / Outlook.com
 * 
 * Flow: Device Code Flow (no web server needed)
 */

const https = require('https');
const readline = require('readline');

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

// Configuration
const config = {
  // For personal Microsoft accounts (Outlook.com, Hotmail, Live)
  tenantId: 'consumers',  // Use 'common' for work/school accounts
  
  // You need to register an app at:
  // https://portal.azure.com/#blade/Microsoft_AAD_RegisteredApps/ApplicationsListBlade
  clientId: process.env.AZURE_CLIENT_ID || '',
  
  // Scopes needed for IMAP/SMTP
  scopes: [
    'https://outlook.office365.com/IMAP.AccessAsUser.All',
    'https://outlook.office365.com/SMTP.Send',
    'offline_access'  // For refresh tokens
  ].join(' ')
};

/**
 * Make HTTPS request
 */
function makeRequest(options, data = null) {
  return new Promise((resolve, reject) => {
    const req = https.request(options, (res) => {
      let body = '';
      res.on('data', chunk => body += chunk);
      res.on('end', () => {
        try {
          resolve(JSON.parse(body));
        } catch (e) {
          resolve(body);
        }
      });
    });
    
    req.on('error', reject);
    
    if (data) {
      req.write(data);
    }
    
    req.end();
  });
}

/**
 * Step 1: Request device code
 */
async function requestDeviceCode() {
  const tokenEndpoint = `https://login.microsoftonline.com/${config.tenantId}/oauth2/v2.0/devicecode`;
  
  const postData = new URLSearchParams({
    client_id: config.clientId,
    scope: config.scopes
  }).toString();
  
  const options = {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded',
      'Content-Length': Buffer.byteLength(postData)
    }
  };
  
  const url = new URL(tokenEndpoint);
  options.hostname = url.hostname;
  options.path = url.pathname;
  
  console.log('\n📡 Requesting device code from Microsoft...\n');
  
  const response = await makeRequest(options, postData);
  
  if (response.error) {
    throw new Error(`Failed to get device code: ${response.error_description || response.error}`);
  }
  
  return response;
}

/**
 * Step 2: Poll for access token
 */
async function pollForToken(deviceCode, interval = 5) {
  const tokenEndpoint = `https://login.microsoftonline.com/${config.tenantId}/oauth2/v2.0/token`;
  
  const postData = new URLSearchParams({
    client_id: config.clientId,
    grant_type: 'urn:ietf:params:oauth:grant-type:device_code',
    device_code: deviceCode
  }).toString();
  
  const options = {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded',
      'Content-Length': Buffer.byteLength(postData)
    }
  };
  
  const url = new URL(tokenEndpoint);
  options.hostname = url.hostname;
  options.path = url.pathname;
  
  console.log('⏳ Waiting for you to authenticate...\n');
  
  while (true) {
    await new Promise(resolve => setTimeout(resolve, interval * 1000));
    
    const response = await makeRequest(options, postData);
    
    if (response.access_token) {
      return response;
    }
    
    if (response.error === 'authorization_pending') {
      process.stdout.write('.');
      continue;
    }
    
    if (response.error === 'slow_down') {
      interval += 5;
      continue;
    }
    
    if (response.error) {
      throw new Error(`Authentication failed: ${response.error_description || response.error}`);
    }
  }
}

/**
 * Save token to file
 */
function saveToken(tokenResponse) {
  const fs = require('fs');
  const path = require('path');
  
  const tokenFile = path.join(__dirname, '../.oauth_token.json');
  const tokenData = {
    access_token: tokenResponse.access_token,
    refresh_token: tokenResponse.refresh_token,
    expires_in: tokenResponse.expires_in,
    token_type: tokenResponse.token_type,
    scope: tokenResponse.scope,
    acquired_at: Date.now()
  };
  
  fs.writeFileSync(tokenFile, JSON.stringify(tokenData, null, 2));
  fs.chmodSync(tokenFile, 0o600);  // Read/write only by owner
  
  console.log(`\n✅ Token saved to: ${tokenFile}`);
  console.log(`   Expires in: ${tokenResponse.expires_in} seconds (~${Math.floor(tokenResponse.expires_in / 3600)} hours)`);
  
  // Also create .env snippet
  const envSnippet = `
# OAuth 2.0 Configuration (add to your .env or config/pens.conf)
PENS_AUTH_METHOD=oauth
PENS_OAUTH_ACCESS_TOKEN=${tokenResponse.access_token}
PENS_OAUTH_REFRESH_TOKEN=${tokenResponse.refresh_token}
`;
  
  const envFile = path.join(__dirname, '../.oauth_env_snippet');
  fs.writeFileSync(envFile, envSnippet);
  
  console.log(`\n📝 Environment snippet saved to: ${envFile}`);
  console.log('\n   Add these lines to your config/pens.conf or .env file');
}

/**
 * Main function
 */
async function main() {
  console.log(`
╔═══════════════════════════════════════════════════════════╗
║                                                           ║
║       Microsoft 365 OAuth 2.0 Token Helper               ║
║       For PENS Email Service                             ║
║                                                           ║
╚═══════════════════════════════════════════════════════════╝
  `);
  
  // Check if client ID is configured
  if (!config.clientId) {
    console.log('❌ Error: Azure AD Client ID not configured!\n');
    console.log('You need to:');
    console.log('1. Register an application at: https://portal.azure.com');
    console.log('2. Under "Azure Active Directory" → "App registrations"');
    console.log('3. Create a new registration for PENS');
    console.log('4. Note the "Application (client) ID"');
    console.log('5. Under "API permissions", add:');
    console.log('   - IMAP.AccessAsUser.All');
    console.log('   - SMTP.Send');
    console.log('   - offline_access');
    console.log('6. Grant admin consent for your organization');
    console.log('7. Run this script with:');
    console.log('   AZURE_CLIENT_ID=your-client-id node oauth-token-helper.js\n');
    process.exit(1);
  }
  
  try {
    // Step 1: Get device code
    const deviceCodeResponse = await requestDeviceCode();
    
    console.log('📋 Follow these steps:\n');
    console.log(`1. Open this URL in your browser:`);
    console.log(`   ${deviceCodeResponse.verification_uri}\n`);
    console.log(`2. Enter this code when prompted:`);
    console.log(`   \x1b[1;32m${deviceCodeResponse.user_code}\x1b[0m\n`);
    console.log(`3. Sign in with: info@velivolant.io\n`);
    console.log(`4. Grant PENS permission to access your email\n`);
    
    // Step 2: Wait for user to authenticate
    const tokenResponse = await pollForToken(
      deviceCodeResponse.device_code,
      deviceCodeResponse.interval || 5
    );
    
    console.log('\n\n✅ Authentication successful!\n');
    
    // Step 3: Save token
    saveToken(tokenResponse);
    
    console.log('\n🎉 Setup complete!\n');
    console.log('Next steps:');
    console.log('1. Add the OAuth configuration to your config/pens.conf');
    console.log('2. Build PENS: make clean && make');
    console.log('3. Run PENS with OAuth: ./pens -c config/pens.conf');
    console.log('\nYour access token is valid for ~1 hour.');
    console.log('Use the refresh token to get new access tokens without re-authenticating.\n');
    
  } catch (error) {
    console.error('\n❌ Error:', error.message);
    process.exit(1);
  } finally {
    rl.close();
  }
}

// Run main function
main();

