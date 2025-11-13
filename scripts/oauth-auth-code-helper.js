#!/usr/bin/env node
/**
 * OAuth 2.0 Authorization Code Flow Helper for Microsoft 365
 * ==========================================================
 * 
 * Alternative to device code flow - uses Authorization Code Flow with PKCE
 * Works better with certificate-based authentication
 * 
 * Usage:
 *   node scripts/oauth-auth-code-helper.js
 */

const https = require('https');
const http = require('http');
const readline = require('readline');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { exec } = require('child_process');
const { promisify } = require('util');

const execAsync = promisify(exec);

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

/**
 * Read configuration from pens.conf file
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
        if (!trimmed || trimmed.startsWith('#')) continue;
        
        const match = trimmed.match(/^([^=]+?)\s*=\s*(.+)$/);
        if (match) {
          const key = match[1].trim();
          const value = match[2].trim();
          
          if (key === 'oauth_client_id') config.clientId = value;
          else if (key === 'oauth_tenant_id') config.tenantId = value;
          else if (key === 'oauth_scope') config.scopes = value;
          else if (key === 'oauth_client_secret') config.clientSecret = value;
          else if (key === 'oauth_certificate_path') config.certificatePath = value;
          else if (key === 'oauth_private_key_path') config.privateKeyPath = value;
        }
      }
    }
  } catch (error) {
    // Silently fail
  }
  
  return config;
}

const fileConfig = readConfigFile();

const config = {
  tenantId: process.env.AZURE_TENANT_ID || fileConfig.tenantId || 'common',
  clientId: process.env.AZURE_CLIENT_ID || fileConfig.clientId || '',
  scopes: process.env.AZURE_SCOPES || fileConfig.scopes || [
    'https://outlook.office365.com/IMAP.AccessAsUser.All',
    'https://outlook.office365.com/Mail.Send',
    'offline_access'
  ].join(' '),
  certificatePath: process.env.AZURE_CERTIFICATE_PATH || process.env.PENS_OAUTH_CERTIFICATE_PATH || fileConfig.certificatePath || '',
  privateKeyPath: process.env.AZURE_PRIVATE_KEY_PATH || process.env.PENS_OAUTH_PRIVATE_KEY_PATH || fileConfig.privateKeyPath || '',
  clientSecret: process.env.AZURE_CLIENT_SECRET || fileConfig.clientSecret || '',
  redirectUri: 'http://localhost:8080/oauth/callback'
};

/**
 * Generate PKCE code verifier and challenge
 */
function generatePKCE() {
  const codeVerifier = crypto.randomBytes(32).toString('base64url');
  const codeChallenge = crypto.createHash('sha256')
    .update(codeVerifier)
    .digest('base64url');
  
  return { codeVerifier, codeChallenge };
}

/**
 * Generate client assertion JWT (same as in oauth-token-helper.js)
 */
function generateClientAssertion(clientId, tenantId, certificatePath, privateKeyPath) {
  try {
    if (!fs.existsSync(certificatePath) || !fs.existsSync(privateKeyPath)) {
      return null;
    }
    
    const privateKeyPEM = fs.readFileSync(privateKeyPath, 'utf8');
    const certPEM = fs.readFileSync(certificatePath, 'utf8');
    
    // Calculate thumbprint
    const certBase64 = certPEM
      .replace(/-----BEGIN CERTIFICATE-----/g, '')
      .replace(/-----END CERTIFICATE-----/g, '')
      .replace(/\n/g, '')
      .replace(/\r/g, '')
      .trim();
    
    const certBuffer = Buffer.from(certBase64, 'base64');
    const hash = crypto.createHash('sha1').update(certBuffer).digest();
    // Azure AD x5t requires base64url encoding (no padding)
    const thumbprintBase64 = hash.toString('base64')
      .replace(/\+/g, '-')
      .replace(/\//g, '_')
      .replace(/=/g, '');
    
    // Build JWT
    const now = Math.floor(Date.now() / 1000);
    const header = { alg: 'RS256', typ: 'JWT', x5t: thumbprintBase64 };
    const payload = {
      aud: `https://login.microsoftonline.com/${tenantId}/oauth2/v2.0/token`,
      exp: now + 3600,
      iss: clientId,
      jti: `${now.toString(16)}-${Math.random().toString(36).substring(2, 15)}`,
      nbf: now,
      sub: clientId
    };
    
    const base64UrlEncode = (str) => Buffer.from(str)
      .toString('base64')
      .replace(/\+/g, '-')
      .replace(/\//g, '_')
      .replace(/=/g, '');
    
    const headerB64 = base64UrlEncode(JSON.stringify(header));
    const payloadB64 = base64UrlEncode(JSON.stringify(payload));
    const signatureInput = `${headerB64}.${payloadB64}`;
    
    // Sign with private key - must use the exact same key that matches the certificate
    const sign = crypto.createSign('RSA-SHA256');
    sign.update(signatureInput, 'utf8');
    sign.end();
    
    // Sign and get raw signature buffer
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
    
    return `${headerB64}.${payloadB64}.${signatureB64}`;
  } catch (error) {
    console.error('Error generating client assertion:', error.message);
    return null;
  }
}

/**
 * Start local HTTP server to receive callback
 * Returns a promise that resolves with the authorization code
 */
function startCallbackServer() {
  return new Promise((resolve, reject) => {
    let codeResolved = false;
    
    const server = http.createServer((req, res) => {
      const url = new URL(req.url, `http://${req.headers.host}`);
      
      if (url.pathname === '/oauth/callback') {
        const code = url.searchParams.get('code');
        const error = url.searchParams.get('error');
        const errorDescription = url.searchParams.get('error_description');
        
        res.writeHead(200, { 'Content-Type': 'text/html' });
        
        if (error) {
          res.end(`
            <html>
              <body style="font-family: Arial, sans-serif; padding: 20px;">
                <h1>❌ Authentication Error</h1>
                <p><strong>Error:</strong> ${error}</p>
                <p><strong>Description:</strong> ${errorDescription || 'No description provided'}</p>
                <p>You can close this window and check the terminal for instructions.</p>
              </body>
            </html>
          `);
          server.close();
          if (!codeResolved) {
            codeResolved = true;
            reject(new Error(errorDescription || error));
          }
          return;
        }
        
        if (code) {
          res.end(`
            <html>
              <body style="font-family: Arial, sans-serif; padding: 20px; text-align: center;">
                <h1>✅ Authentication Successful!</h1>
                <p>You can close this window and return to the terminal.</p>
                <p>The authorization code has been received.</p>
              </body>
            </html>
          `);
          server.close();
          if (!codeResolved) {
            codeResolved = true;
            resolve(code);
          }
        } else {
          res.end(`
            <html>
              <body style="font-family: Arial, sans-serif; padding: 20px;">
                <h1>⚠️ No authorization code received</h1>
                <p>You can close this window.</p>
              </body>
            </html>
          `);
          server.close();
          if (!codeResolved) {
            codeResolved = true;
            reject(new Error('No authorization code in callback'));
          }
        }
      } else {
        res.writeHead(404);
        res.end('Not found');
      }
    });
    
    server.listen(8080, 'localhost', (err) => {
      if (err) {
        if (err.code === 'EADDRINUSE') {
          reject(new Error('Port 8080 is already in use. Please close the application using it or use a different port.'));
        } else {
          reject(err);
        }
      } else {
        console.log('✓ Local callback server started on http://localhost:8080');
      }
    });
    
    server.on('error', (err) => {
      if (!codeResolved) {
        codeResolved = true;
        if (err.code === 'EADDRINUSE') {
          reject(new Error('Port 8080 is already in use. Please close the application using it.'));
        } else {
          reject(err);
        }
      }
    });
    
    // Timeout after 10 minutes
    setTimeout(() => {
      if (!codeResolved) {
        codeResolved = true;
        server.close();
        reject(new Error('Callback server timeout - no response received within 10 minutes'));
      }
    }, 600000);
  });
}

/**
 * Exchange authorization code for tokens
 */
async function exchangeCodeForTokens(authorizationCode, codeVerifier) {
  const tokenEndpoint = `https://login.microsoftonline.com/${config.tenantId}/oauth2/v2.0/token`;
  
  const postDataParams = {
    client_id: config.clientId,
    grant_type: 'authorization_code',
    code: authorizationCode,
    redirect_uri: config.redirectUri,
    code_verifier: codeVerifier
  };
  
  // Use certificate if available, otherwise client secret
  const certExists = config.certificatePath && config.privateKeyPath && 
                     fs.existsSync(config.certificatePath) && fs.existsSync(config.privateKeyPath);
  
  if (certExists) {
    try {
      const clientAssertion = generateClientAssertion(
        config.clientId, config.tenantId, config.certificatePath, config.privateKeyPath
      );
      if (clientAssertion && clientAssertion.length > 0) {
        postDataParams.client_assertion_type = 'urn:ietf:params:oauth:client-assertion-type:jwt-bearer';
        postDataParams.client_assertion = clientAssertion;
        console.log('✓ Using certificate for token exchange');
        console.log(`   Client assertion length: ${clientAssertion.length} characters`);
      } else {
        throw new Error('Failed to generate client assertion - assertion is empty or null');
      }
    } catch (error) {
      console.error(`⚠️  Failed to use certificate: ${error.message}`);
      if (error.stack) {
        console.error(`   Stack: ${error.stack.split('\n').slice(0, 3).join('\n')}`);
      }
      if (config.clientSecret && config.clientSecret.trim() !== '') {
        postDataParams.client_secret = config.clientSecret.trim();
        console.log('✓ Using client secret for token exchange (certificate failed)');
      } else {
        throw new Error('Certificate failed and no client secret available');
      }
    }
  } else if (config.clientSecret && config.clientSecret.trim() !== '') {
    postDataParams.client_secret = config.clientSecret.trim();
    console.log('✓ Using client secret for token exchange');
  } else {
    throw new Error('Neither certificate nor client secret configured');
  }
  
  const postData = new URLSearchParams(postDataParams).toString();
  
  return new Promise((resolve, reject) => {
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
    
    const req = https.request(options, (res) => {
      let body = '';
      res.on('data', chunk => body += chunk);
      res.on('end', () => {
        try {
          const parsed = JSON.parse(body);
          if (res.statusCode >= 200 && res.statusCode < 300) {
            resolve(parsed);
          } else {
            reject(new Error(`HTTP ${res.statusCode}: ${parsed.error_description || parsed.error || body}`));
          }
        } catch (e) {
          reject(new Error(`Invalid response: ${body}`));
        }
      });
    });
    
    req.on('error', reject);
    req.write(postData);
    req.end();
  });
}

/**
 * Save token to file
 */
function saveToken(tokenResponse) {
  const tokenFile = path.join(__dirname, '../.oauth_token.json');
  
  if (!tokenResponse.access_token) {
    throw new Error('Invalid token response: missing access_token');
  }
  
  const tokenData = {
    access_token: tokenResponse.access_token,
    refresh_token: tokenResponse.refresh_token || '',
    expires_in: tokenResponse.expires_in || 3600,
    token_type: tokenResponse.token_type || 'Bearer',
    scope: tokenResponse.scope || config.scopes,
    acquired_at: Date.now()
  };
  
  const tokenDir = path.dirname(tokenFile);
  if (!fs.existsSync(tokenDir)) {
    fs.mkdirSync(tokenDir, { recursive: true });
  }
  
  fs.writeFileSync(tokenFile, JSON.stringify(tokenData, null, 2));
  fs.chmodSync(tokenFile, 0o600);
  
  console.log(`\n✅ Token saved to: ${tokenFile}`);
  console.log(`   Expires in: ${tokenData.expires_in} seconds (~${Math.floor(tokenData.expires_in / 3600)} hours)`);
  
  if (tokenData.refresh_token) {
    console.log(`   Refresh token: ${tokenData.refresh_token.substring(0, 20)}...`);
  }
}

/**
 * Open browser (cross-platform)
 */
async function openBrowser(url) {
  const platform = process.platform;
  let command;
  
  if (platform === 'darwin') {
    command = `open "${url}"`;
  } else if (platform === 'win32') {
    command = `start "" "${url}"`;
  } else {
    command = `xdg-open "${url}"`;
  }
  
  try {
    await execAsync(command);
    return true;
  } catch (error) {
    return false;
  }
}

/**
 * Main function
 */
async function main() {
  console.log(`
╔═══════════════════════════════════════════════════════════╗
║                                                           ║
║   Microsoft 365 OAuth 2.0 Authorization Code Flow        ║
║   For PENS Email Service                                  ║
║                                                           ║
╚═══════════════════════════════════════════════════════════╝
  `);
  
  if (!config.clientId || config.clientId.trim() === '') {
    console.log('❌ Error: Azure AD Client ID not configured!\n');
    console.log('Set AZURE_CLIENT_ID environment variable or add oauth_client_id to config/pens.conf\n');
    process.exit(1);
  }
  
  console.log('Configuration:');
  console.log(`  Client ID: ${config.clientId}`);
  console.log(`  Tenant ID: ${config.tenantId}`);
  console.log(`  Scopes: ${config.scopes}`);
  console.log(`  Redirect URI: ${config.redirectUri}`);
  console.log(`\n⚠️  REQUIRED: Add this redirect URI to Azure AD:`);
  console.log(`   1. Go to https://portal.azure.com`);
  console.log(`   2. Azure Active Directory → App registrations`);
  console.log(`   3. Find your app (Client ID: ${config.clientId})`);
  console.log(`   4. Go to "Authentication" → "Add a platform" → "Web"`);
  console.log(`   5. Add redirect URI: ${config.redirectUri}`);
  console.log(`   6. Click "Configure" and save\n`);
  
  // Check certificate
  const certExists = config.certificatePath && config.privateKeyPath && 
                     fs.existsSync(config.certificatePath) && fs.existsSync(config.privateKeyPath);
  
  if (certExists) {
    console.log(`  Certificate: ${config.certificatePath} (PRIORITY)`);
    console.log(`  Private Key: ${config.privateKeyPath}`);
  } else if (config.clientSecret) {
    console.log(`  Client Secret: ${config.clientSecret.substring(0, 8)}... (fallback)`);
  }
  console.log('');
  
  try {
    // Generate PKCE
    const { codeVerifier, codeChallenge } = generatePKCE();
    
    // Build authorization URL
    const authUrl = new URL(`https://login.microsoftonline.com/${config.tenantId}/oauth2/v2.0/authorize`);
    authUrl.searchParams.set('client_id', config.clientId);
    authUrl.searchParams.set('response_type', 'code');
    authUrl.searchParams.set('redirect_uri', config.redirectUri);
    authUrl.searchParams.set('scope', config.scopes);
    authUrl.searchParams.set('response_mode', 'query');
    authUrl.searchParams.set('code_challenge', codeChallenge);
    authUrl.searchParams.set('code_challenge_method', 'S256');
    authUrl.searchParams.set('prompt', 'consent');
    
    // Start callback server (this returns a promise that resolves with the auth code)
    console.log('📡 Starting local callback server...');
    
    // Start server and wait for callback in parallel
    const callbackPromise = startCallbackServer();
    
    console.log('\n📋 Opening browser for authentication...\n');
    console.log('⚠️  IMPORTANT: Make sure this redirect URI is registered in Azure AD:');
    console.log(`   ${config.redirectUri}\n`);
    console.log('If browser doesn\'t open automatically, visit this URL:');
    console.log(`   ${authUrl.toString()}\n`);
    
    // Open browser
    const browserOpened = await openBrowser(authUrl.toString());
    if (!browserOpened) {
      console.log('⚠️  Could not open browser automatically.');
      console.log(`   Please visit: ${authUrl.toString()}\n`);
    }
    
    console.log('⏳ Waiting for authentication...\n');
    console.log('   (The server will wait up to 10 minutes for your response)\n');
    
    // Wait for callback (startCallbackServer promise resolves with the code)
    const authorizationCode = await callbackPromise;
    
    console.log('✓ Authorization code received\n');
    console.log('🔄 Exchanging code for tokens...\n');
    
    // Exchange code for tokens
    const tokenResponse = await exchangeCodeForTokens(authorizationCode, codeVerifier);
    
    console.log('✅ Token exchange successful!\n');
    
    // Save tokens
    saveToken(tokenResponse);
    
    console.log('\n🎉 Setup complete!\n');
    console.log('Next steps:');
    console.log('1. Build PENS: cd pens && make clean && make');
    console.log('2. Run PENS: ./pens -c config/pens.conf');
    console.log('\nYour access token is valid for ~1 hour.');
    console.log('PENS will automatically refresh tokens using your certificate.\n');
    
  } catch (error) {
    console.error('\n❌ Error:', error.message);
    
    if (error.message.includes('AADSTS500113') || error.message.includes('reply address') || error.message.includes('redirect_uri')) {
      console.error('\n💡 Solution: Register the redirect URI in Azure AD');
      console.error('   1. Go to https://portal.azure.com');
      console.error('   2. Azure Active Directory → App registrations');
      console.error(`   3. Find your app (Client ID: ${config.clientId})`);
      console.error('   4. Go to "Authentication" → "Add a platform" → "Web"');
      console.error(`   5. Add redirect URI: ${config.redirectUri}`);
      console.error('   6. Click "Configure" and save');
      console.error('   7. Wait 1-2 minutes for changes to propagate');
      console.error('   8. Run this script again\n');
    } else if (error.message.includes('EADDRINUSE') || error.message.includes('Port 8080')) {
      console.error('\n💡 Solution: Port 8080 is already in use');
      console.error('   - Close the application using port 8080');
      console.error('   - Or modify the script to use a different port\n');
    }
    
    process.exit(1);
  } finally {
    rl.close();
  }
}

// Run if executed directly
if (require.main === module) {
  main();
}

module.exports = { generateClientAssertion, exchangeCodeForTokens };

