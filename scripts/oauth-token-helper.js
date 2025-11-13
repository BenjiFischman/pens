#!/usr/bin/env node
/**
 * OAuth 2.0 Token Helper for Microsoft 365
 * ========================================
 * 
 * This script helps you get OAuth 2.0 access tokens for PENS
 * to authenticate with Microsoft 365 / Outlook.com
 * 
 * Flow: Device Code Flow (no web server needed)
 * 
 * Alternative: Use oauth-auth-code-helper.js for Authorization Code Flow
 *              which works better with certificate-based authentication
 */

const https = require('https');
const readline = require('readline');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

/**
 * Read configuration from pens.conf file if it exists
 */
function readConfigFile() {
  const configPath = path.join(__dirname, '../config/pens.conf');
  const config = {};
  
  try {
    if (fs.existsSync(configPath)) {
      const content = fs.readFileSync(configPath, 'utf8');
      const lines = content.split('\n');
      
      for (const line of lines) {
        const trimmed = line.trim();
        // Skip comments and empty lines
        if (!trimmed || trimmed.startsWith('#')) continue;
        
        const match = trimmed.match(/^([^=]+?)\s*=\s*(.+)$/);
        if (match) {
          const key = match[1].trim();
          const value = match[2].trim();
          
          if (key === 'oauth_client_id') {
            config.clientId = value;
          } else if (key === 'oauth_tenant_id') {
            config.tenantId = value;
          } else if (key === 'oauth_scope') {
            config.scopes = value;
          } else if (key === 'oauth_client_secret') {
            config.clientSecret = value;
          } else if (key === 'oauth_certificate_path') {
            config.certificatePath = value;
          } else if (key === 'oauth_private_key_path') {
            config.privateKeyPath = value;
          }
        }
      }
    }
  } catch (error) {
    // Silently fail - will use defaults
  }
  
  return config;
}

// Read config from file first, then override with environment variables
const fileConfig = readConfigFile();

  // Configuration with priority: env vars > config file > defaults
  // Certificate paths from config file use PENS_ prefix in env vars
  const config = {
    // Tenant configuration
    // - Use AZURE_TENANT_ID environment variable to override
    // - "common" works for multi-tenant apps
    // - "consumers" for personal Microsoft accounts
    // - "organizations" for work/school accounts
    tenantId: process.env.AZURE_TENANT_ID || fileConfig.tenantId || 'common',
    
    // Azure AD application (client) ID
    clientId: process.env.AZURE_CLIENT_ID || fileConfig.clientId || '',
    
    // Scopes required for IMAP/SMTP access
    scopes: process.env.AZURE_SCOPES || fileConfig.scopes || [
      'https://outlook.office365.com/IMAP.AccessAsUser.All',
      'https://outlook.office365.com/Mail.Send',
      'offline_access'
    ].join(' '),
    
    // Certificate-based authentication (PRIORITY - checked first)
    certificatePath: process.env.AZURE_CERTIFICATE_PATH || process.env.PENS_OAUTH_CERTIFICATE_PATH || fileConfig.certificatePath || '',
    privateKeyPath: process.env.AZURE_PRIVATE_KEY_PATH || process.env.PENS_OAUTH_PRIVATE_KEY_PATH || fileConfig.privateKeyPath || '',
    
    // Azure AD application client secret (fallback only)
    clientSecret: process.env.AZURE_CLIENT_SECRET || fileConfig.clientSecret || ''
  };

/**
 * Make HTTPS request with timeout
 */
function makeRequest(options, data = null, timeout = 30000) {
  return new Promise((resolve, reject) => {
    const req = https.request(options, (res) => {
      let body = '';
      res.on('data', chunk => body += chunk);
      res.on('end', () => {
        try {
          const parsed = JSON.parse(body);
          // Include status code for error handling
          parsed._statusCode = res.statusCode;
          resolve(parsed);
        } catch (e) {
          // If not JSON, return raw body with status code
          resolve({ _statusCode: res.statusCode, _rawBody: body });
        }
      });
    });
    
    req.on('error', reject);
    req.setTimeout(timeout, () => {
      req.destroy();
      reject(new Error(`Request timeout after ${timeout}ms`));
    });
    
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
  
  const postDataParams = {
    client_id: config.clientId,
    scope: config.scopes
  };
  
  // Device code endpoint typically only accepts client_secret, not client_assertion
  // Certificate will be used for token polling instead
  if (config.clientSecret && config.clientSecret.trim() !== '') {
    postDataParams.client_secret = config.clientSecret.trim();
    console.log('✓ Using client secret for device code request (certificate will be used for token exchange)');
  } else {
    console.log('⚠️  No client secret - device code request may fail for confidential clients');
  }
  
  const postData = new URLSearchParams(postDataParams).toString();
  
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
  
  let response;
  try {
    response = await makeRequest(options, postData);
  } catch (error) {
    throw new Error(`Network error: ${error.message}. Please check your internet connection.`);
  }
  
  // Check for HTTP errors
  if (response._statusCode && (response._statusCode < 200 || response._statusCode >= 300)) {
    // Parse error from response body if available
    const errorCode = response.error || '';
    const errorDesc = response.error_description || response._rawBody || 'Unknown error';
    
    // Check for AADSTS7000218 in error description
    if (errorDesc.includes('AADSTS7000218') || errorDesc.includes('client_assertion') || errorDesc.includes('client_secret')) {
      let errorMsg = `HTTP ${response._statusCode}: ${errorDesc}`;
      errorMsg += '\n\n❌ Application is configured as a CONFIDENTIAL client, but device code flow requires a PUBLIC client.';
      errorMsg += '\n\n🔧 Fix this in Azure Portal:';
      errorMsg += '\n  1. Go to https://portal.azure.com';
      errorMsg += '\n  2. Navigate to: Azure Active Directory → App registrations';
      errorMsg += `\n  3. Find your app (Client ID: ${config.clientId})`;
      errorMsg += '\n  4. Go to "Authentication" (left sidebar)';
      errorMsg += '\n  5. Under "Advanced settings", find "Allow public client flows"';
      errorMsg += '\n  6. Set it to "Yes"';
      errorMsg += '\n  7. Click "Save"';
      errorMsg += '\n  8. Wait 1-2 minutes for changes to propagate';
      errorMsg += '\n  9. Run this script again';
      throw new Error(errorMsg);
    }
    
    throw new Error(`HTTP ${response._statusCode}: ${errorDesc}`);
  }
  
  if (response.error) {
    let errorMsg = `Failed to get device code: ${response.error_description || response.error}`;
    
    // Provide helpful messages for common errors
    if (response.error === 'AADSTS700016') {
      errorMsg += '\n\n❌ Application not found in tenant.';
      errorMsg += '\n\nPossible causes:';
      errorMsg += `\n  1. Client ID is incorrect: ${config.clientId}`;
      errorMsg += `\n  2. Application is not registered in tenant: ${config.tenantId}`;
      errorMsg += '\n  3. Tenant ID does not match the tenant where the app is registered';
      errorMsg += '\n\nSolution:';
      errorMsg += '\n  - Verify the Client ID in Azure Portal (App registrations)';
      errorMsg += '\n  - Check the Tenant ID matches where the app is registered';
      errorMsg += '\n  - For multi-tenant apps, use "common" or "organizations"';
      errorMsg += '\n  - For single-tenant apps, use the specific Tenant ID';
    } else if (response.error === 'AADSTS50059') {
      errorMsg += '\n\n❌ No tenant-identifying information found.';
      errorMsg += '\n\nSolution:';
      errorMsg += '\n  - For single-tenant apps, provide the specific Tenant ID';
      errorMsg += '\n  - Set AZURE_TENANT_ID environment variable or update tenantId in script';
    } else if (response.error === 'AADSTS9002346') {
      errorMsg += '\n\n❌ Application configured for Microsoft Account users only.';
      errorMsg += '\n\nSolution:';
      errorMsg += '\n  - Use "consumers" as tenantId for personal Microsoft accounts';
      errorMsg += '\n  - Or change app registration to support both personal and work accounts';
    } else if (response.error === 'AADSTS7000218' || response.error_description?.includes('client_assertion') || response.error_description?.includes('client_secret')) {
      errorMsg += '\n\n❌ Application is configured as a CONFIDENTIAL client, but device code flow requires a PUBLIC client.';
      errorMsg += '\n\n🔧 Fix this in Azure Portal:';
      errorMsg += '\n  1. Go to https://portal.azure.com';
      errorMsg += '\n  2. Navigate to: Azure Active Directory → App registrations';
      errorMsg += `\n  3. Find your app (Client ID: ${config.clientId})`;
      errorMsg += '\n  4. Go to "Authentication" (left sidebar)';
      errorMsg += '\n  5. Under "Advanced settings", find "Allow public client flows"';
      errorMsg += '\n  6. Set it to "Yes"';
      errorMsg += '\n  7. Click "Save"';
      errorMsg += '\n  8. Wait 1-2 minutes for changes to propagate';
      errorMsg += '\n  9. Run this script again';
      errorMsg += '\n\n💡 Why?';
      errorMsg += '\n  - Device code flow is for public clients (no client secret)';
      errorMsg += '\n  - Your app is currently set as confidential (requires secret)';
      errorMsg += '\n  - Enabling public client flows allows device code authentication';
    }
    
    throw new Error(errorMsg);
  }
  
  return response;
}

/**
 * Step 2: Poll for access token
 */
async function pollForToken(deviceCode, interval = 5, expiresIn = 900) {
  const tokenEndpoint = `https://login.microsoftonline.com/${config.tenantId}/oauth2/v2.0/token`;
  
  const postDataParams = {
    client_id: config.clientId,
    grant_type: 'urn:ietf:params:oauth:grant-type:device_code',
    device_code: deviceCode
  };
  
  // PRIORITIZE certificate - use it if available, otherwise fall back to client secret
  const certExists = config.certificatePath && config.privateKeyPath && 
                     fs.existsSync(config.certificatePath) && fs.existsSync(config.privateKeyPath);
  
  if (certExists) {
    // Use certificate-based authentication (PRIORITY)
    try {
      const clientAssertion = generateClientAssertion(config.clientId, config.tenantId, 
                                                      config.certificatePath, config.privateKeyPath);
      if (clientAssertion && clientAssertion.length > 0) {
        postDataParams.client_assertion_type = 'urn:ietf:params:oauth:client-assertion-type:jwt-bearer';
        postDataParams.client_assertion = clientAssertion;
        console.log('✓ Using certificate for token request (PRIORITY)');
        console.log(`   Client assertion length: ${clientAssertion.length} characters`);
      } else {
        throw new Error('Failed to generate client assertion - assertion is empty or null');
      }
    } catch (error) {
      console.error(`⚠️  Failed to use certificate: ${error.message}`);
      if (error.stack) {
        console.error(`   Stack: ${error.stack.split('\n').slice(0, 3).join('\n')}`);
      }
      console.error('   Falling back to client secret...');
      // Fall back to client secret if certificate fails
      if (config.clientSecret && config.clientSecret.trim() !== '') {
        postDataParams.client_secret = config.clientSecret.trim();
        console.log('✓ Using client secret for token request (fallback)');
      } else {
        throw new Error('Certificate failed and no client secret available');
      }
    }
  } else if (config.clientSecret && config.clientSecret.trim() !== '') {
    postDataParams.client_secret = config.clientSecret.trim();
    console.log('✓ Using client secret for token request (certificate not available)');
  } else {
    console.log('⚠️  No certificate or client secret configured - this may fail for confidential clients');
  }
  
  const postData = new URLSearchParams(postDataParams).toString();
  
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
  console.log(`   (This will timeout after ${Math.floor(expiresIn / 60)} minutes)\n`);
  
  const startTime = Date.now();
  const timeoutMs = expiresIn * 1000;
  let pollCount = 0;
  
  while (true) {
    // Check if we've exceeded the device code expiration time
    const elapsed = Date.now() - startTime;
    if (elapsed > timeoutMs) {
      throw new Error(`Device code expired after ${Math.floor(expiresIn / 60)} minutes. Please run the script again to get a new code.`);
    }
    
    await new Promise(resolve => setTimeout(resolve, interval * 1000));
    pollCount++;
    
    // Show progress every 10 polls
    if (pollCount % 10 === 0) {
      const remaining = Math.floor((timeoutMs - elapsed) / 1000);
      process.stdout.write(`\n   (${Math.floor(remaining / 60)}m ${remaining % 60}s remaining) `);
    }
    
    let response;
    try {
      response = await makeRequest(options, postData);
    } catch (error) {
      // Network errors - retry
      if (error.message.includes('timeout') || error.message.includes('ECONNREFUSED')) {
        process.stdout.write('!');
        continue;
      }
      throw error;
    }
    
    // Check for HTTP errors
    if (response._statusCode && (response._statusCode < 200 || response._statusCode >= 300)) {
      if (response._statusCode === 400 && response.error === 'authorization_pending') {
        process.stdout.write('.');
        continue;
      }
      throw new Error(`HTTP ${response._statusCode}: ${response.error_description || response.error || 'Unknown error'}`);
    }
    
    if (response.access_token) {
      // Validate token response - access_token is required, refresh_token is optional
      if (!response.access_token) {
        throw new Error('Invalid token response: missing access_token');
      }
      return response;
    }
    
    if (response.error === 'authorization_pending') {
      process.stdout.write('.');
      continue;
    }
    
    if (response.error === 'slow_down') {
      interval += 5;
      console.log(`\n   (Rate limited, slowing down to ${interval}s intervals)`);
      continue;
    }
    
    if (response.error === 'expired_token') {
      throw new Error('Device code expired. Please run the script again to get a new code.');
    }
    
    if (response.error) {
      let errorMsg = `Authentication failed: ${response.error_description || response.error}`;
      
      // Provide helpful messages for common errors
      if (response.error === 'AADSTS700016') {
        errorMsg += '\n\n❌ Application not found in tenant.';
        errorMsg += '\n\nPossible causes:';
        errorMsg += `\n  1. Client ID is incorrect: ${config.clientId}`;
        errorMsg += `\n  2. Application is not registered in tenant: ${config.tenantId}`;
        errorMsg += '\n  3. Tenant ID does not match the tenant where the app is registered';
        errorMsg += '\n\nSolution:';
        errorMsg += '\n  - Verify the Client ID in Azure Portal (App registrations)';
        errorMsg += '\n  - Check the Tenant ID matches where the app is registered';
        errorMsg += '\n  - For multi-tenant apps, use "common" or "organizations"';
        errorMsg += '\n  - For single-tenant apps, use the specific Tenant ID';
      } else if (response.error === 'invalid_grant') {
        errorMsg += '\n\n❌ Invalid device code.';
        errorMsg += '\n\nSolution: Run the script again to get a new device code.';
      }
      
      throw new Error(errorMsg);
    }
  }
}

/**
 * Save token to file
 */
function saveToken(tokenResponse) {
  const tokenFile = path.join(__dirname, '../.oauth_token.json');
  
  // Validate token response
  if (!tokenResponse.access_token) {
    throw new Error('Invalid token response: missing access_token');
  }
  
  if (!tokenResponse.refresh_token) {
    console.warn('⚠️  Warning: No refresh token received. Token cannot be automatically refreshed.');
  }
  
  const tokenData = {
    access_token: tokenResponse.access_token,
    refresh_token: tokenResponse.refresh_token || '',
    expires_in: tokenResponse.expires_in || 3600,
    token_type: tokenResponse.token_type || 'Bearer',
    scope: tokenResponse.scope || config.scopes,
    acquired_at: Date.now()
  };
  
  try {
    // Ensure directory exists
    const tokenDir = path.dirname(tokenFile);
    if (!fs.existsSync(tokenDir)) {
      fs.mkdirSync(tokenDir, { recursive: true });
    }
    
    fs.writeFileSync(tokenFile, JSON.stringify(tokenData, null, 2));
    fs.chmodSync(tokenFile, 0o600);  // Read/write only by owner
    
    console.log(`\n✅ Token saved to: ${tokenFile}`);
    console.log(`   Expires in: ${tokenData.expires_in} seconds (~${Math.floor(tokenData.expires_in / 3600)} hours)`);
    
    if (tokenData.refresh_token) {
      console.log(`   Refresh token: ${tokenData.refresh_token.substring(0, 20)}...`);
    }
  } catch (error) {
    throw new Error(`Failed to save token file: ${error.message}`);
  }
  
  // Also create .env snippet (optional)
  try {
    const envSnippet = `# OAuth 2.0 Configuration (add to your .env or config/pens.conf)
PENS_AUTH_METHOD=oauth
PENS_OAUTH_ACCESS_TOKEN=${tokenResponse.access_token}
PENS_OAUTH_REFRESH_TOKEN=${tokenResponse.refresh_token || ''}
`;
    
    const envFile = path.join(__dirname, '../.oauth_env_snippet');
    fs.writeFileSync(envFile, envSnippet);
    
    console.log(`\n📝 Environment snippet saved to: ${envFile}`);
    console.log('   (This file is optional - tokens are already in .oauth_token.json)');
  } catch (error) {
    // Non-critical - just warn
    console.warn(`\n⚠️  Could not save env snippet: ${error.message}`);
  }
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
  if (!config.clientId || config.clientId.trim() === '') {
    console.log('❌ Error: Azure AD Client ID not configured!\n');
    console.log('You need to configure the Client ID in one of these ways:\n');
    console.log('Option 1: Set environment variable');
    console.log('   AZURE_CLIENT_ID=your-client-id node scripts/oauth-token-helper.js\n');
    console.log('Option 2: Add to config/pens.conf');
    console.log('   oauth_client_id = your-client-id\n');
    console.log('To get a Client ID:');
    console.log('1. Go to https://portal.azure.com');
    console.log('2. Navigate to: Azure Active Directory → App registrations');
    console.log('3. Create a new registration for PENS');
    console.log('4. Note the "Application (client) ID"');
    console.log('5. Go to "Authentication" → Enable "Allow public client flows"');
    console.log('6. Under "API permissions", add:');
    console.log('   - IMAP.AccessAsUser.All');
    console.log('   - SMTP.Send');
    console.log('   - offline_access');
    console.log('7. Grant admin consent for your organization\n');
    process.exit(1);
  }
  
  // Display configuration being used
  console.log('Configuration:');
  console.log(`  Client ID: ${config.clientId}`);
  console.log(`  Tenant ID: ${config.tenantId}`);
  console.log(`  Scopes: ${config.scopes}`);
  
  // Check certificate files exist
  const certExists = config.certificatePath && config.privateKeyPath && 
                     fs.existsSync(config.certificatePath) && fs.existsSync(config.privateKeyPath);
  
  if (certExists) {
    console.log(`  Certificate: ${config.certificatePath} (configured - PRIORITY)`);
    console.log(`  Private Key: ${config.privateKeyPath}`);
    console.log(`  Authentication: Certificate-based (will be used for all requests)`);
  } else if (config.clientSecret && config.clientSecret.trim() !== '') {
    console.log(`  Client Secret: ${config.clientSecret.substring(0, 8)}... (configured - fallback only)`);
    if (config.certificatePath || config.privateKeyPath) {
      console.log(`  ⚠️  Certificate paths configured but files not found:`);
      if (config.certificatePath) console.log(`     Certificate: ${config.certificatePath} ${fs.existsSync(config.certificatePath) ? '✓' : '✗ (not found)'}`);
      if (config.privateKeyPath) console.log(`     Private Key: ${config.privateKeyPath} ${fs.existsSync(config.privateKeyPath) ? '✓' : '✗ (not found)'}`);
    }
  } else {
    console.log(`  Authentication: Device code flow (no certificate or secret)`);
    console.log(`  Note: For automated token refresh, configure certificate or client secret`);
  }
  console.log('');
  
  try {
    // Step 1: Get device code
    const deviceCodeResponse = await requestDeviceCode();
    
    // Validate device code response
    if (!deviceCodeResponse.device_code || !deviceCodeResponse.user_code || !deviceCodeResponse.verification_uri) {
      throw new Error('Invalid device code response from Microsoft');
    }
    
    const expiresIn = deviceCodeResponse.expires_in || 900; // Default 15 minutes
    const interval = deviceCodeResponse.interval || 5;
    
    console.log('📋 Follow these steps:\n');
    console.log(`1. Open this URL in your browser:`);
    console.log(`   \x1b[4;34m${deviceCodeResponse.verification_uri}\x1b[0m\n`);
    console.log(`2. Enter this code when prompted:`);
    console.log(`   \x1b[1;32m${deviceCodeResponse.user_code}\x1b[0m\n`);
    console.log(`3. Sign in with your Microsoft account\n`);
    console.log(`4. Grant PENS permission to access your email\n`);
    console.log(`   (You have ${Math.floor(expiresIn / 60)} minutes to complete this)\n`);
    
    // Step 2: Wait for user to authenticate
    const tokenResponse = await pollForToken(
      deviceCodeResponse.device_code,
      interval,
      expiresIn
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
    
    // Provide additional help for common issues
    if (error.message.includes('ECONNREFUSED') || error.message.includes('timeout')) {
      console.error('\n💡 Tip: Check your internet connection and firewall settings.');
    } else if (error.message.includes('AADSTS')) {
      console.error('\n💡 Tip: Verify your Azure AD app registration settings in the Azure Portal.');
    }
    
    process.exit(1);
  } finally {
    rl.close();
  }
}

/**
 * Generate JWT client assertion for certificate-based authentication
 */
function generateClientAssertion(clientId, tenantId, certificatePath, privateKeyPath) {
  try {
    // Verify files exist
    if (!fs.existsSync(certificatePath)) {
      throw new Error(`Certificate file not found: ${certificatePath}`);
    }
    if (!fs.existsSync(privateKeyPath)) {
      throw new Error(`Private key file not found: ${privateKeyPath}`);
    }
    
    // Read private key
    const privateKeyPEM = fs.readFileSync(privateKeyPath, 'utf8');
    if (!privateKeyPEM.includes('BEGIN') || !privateKeyPEM.includes('PRIVATE KEY')) {
      throw new Error('Invalid private key format - must be PEM format');
    }
    
    // Read certificate to get thumbprint
    const certPEM = fs.readFileSync(certificatePath, 'utf8');
    if (!certPEM.includes('BEGIN CERTIFICATE')) {
      throw new Error('Invalid certificate format - must be PEM format');
    }
    
    const thumbprint = calculateThumbprint(certPEM);
    if (!thumbprint || !thumbprint.base64) {
      throw new Error('Failed to calculate certificate thumbprint');
    }
    
    // Azure AD x5t requires base64url encoding (no padding)
    const thumbprintBase64Url = thumbprint.base64
      .replace(/\+/g, '-')
      .replace(/\//g, '_')
      .replace(/=/g, '');
    
    // Build JWT header
    const header = {
      alg: 'RS256',
      typ: 'JWT',
      x5t: thumbprintBase64Url
    };
    
    // Build JWT payload
    const now = Math.floor(Date.now() / 1000);
    const payload = {
      aud: `https://login.microsoftonline.com/${tenantId}/oauth2/v2.0/token`,
      exp: now + 3600,
      iss: clientId,
      jti: `${now.toString(16)}-${Math.random().toString(36).substring(2, 15)}`,
      nbf: now,
      sub: clientId
    };
    
    // Encode header and payload
    const headerB64 = base64UrlEncode(JSON.stringify(header));
    const payloadB64 = base64UrlEncode(JSON.stringify(payload));
    const signatureInput = `${headerB64}.${payloadB64}`;
    
    // Sign with private key - must use the exact same key that matches the certificate
    const sign = crypto.createSign('RSA-SHA256');
    sign.update(signatureInput, 'utf8');
    sign.end();
    
    // Sign with explicit padding (PKCS1 for RSA)
    const signature = sign.sign({
      key: privateKeyPEM,
      padding: crypto.constants.RSA_PKCS1_PADDING
    });
    
    if (!signature || signature.length === 0) {
      throw new Error('Failed to sign JWT - signature is empty');
    }
    
    // Convert signature buffer directly to base64url (no double encoding)
    // signature is already a Buffer, convert directly to base64url
    const signatureB64 = signature.toString('base64')
      .replace(/\+/g, '-')
      .replace(/\//g, '_')
      .replace(/=/g, '');
    
    const jwt = `${headerB64}.${payloadB64}.${signatureB64}`;
    
    if (!jwt || jwt.length < 100) {
      throw new Error('Generated JWT is too short or invalid');
    }
    
    return jwt;
  } catch (error) {
    console.error(`Error generating client assertion: ${error.message}`);
    if (error.stack) {
      console.error(`Stack: ${error.stack}`);
    }
    return null;
  }
}

/**
 * Calculate certificate thumbprint
 */
function calculateThumbprint(certPEM) {
  try {
    const certBase64 = certPEM
      .replace(/-----BEGIN CERTIFICATE-----/g, '')
      .replace(/-----END CERTIFICATE-----/g, '')
      .replace(/\n/g, '')
      .replace(/\r/g, '')
      .trim();
    
    const certBuffer = Buffer.from(certBase64, 'base64');
    const hash = crypto.createHash('sha1').update(certBuffer).digest();
    const hex = hash.toString('hex').toUpperCase();
    const base64 = hash.toString('base64');
    
    return { hex, base64 };
  } catch (error) {
    return null;
  }
}

/**
 * Base64 URL encoding
 */
function base64UrlEncode(str) {
  return Buffer.from(str)
    .toString('base64')
    .replace(/\+/g, '-')
    .replace(/\//g, '_')
    .replace(/=/g, '');
}

// Run main function
main();

