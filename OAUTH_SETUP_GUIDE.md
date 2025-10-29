# OAuth 2.0 Setup Guide for PENS

## 🔐 Modern, Secure Authentication for Microsoft 365

OAuth 2.0 is the recommended authentication method for Microsoft 365 / Outlook.com email accounts.

**Benefits over App Passwords:**
- ✅ More secure (tokens expire, can be revoked)
- ✅ No password stored in config files
- ✅ Granular permissions (only what's needed)
- ✅ Refresh tokens for long-term use
- ✅ Works with accounts that have Modern Auth enforced

---

## 🚀 Quick Setup (5 Steps)

### Step 1: Register Azure AD Application

1. Go to https://portal.azure.com
2. Navigate to **Azure Active Directory** → **App registrations**
3. Click **+ New registration**
4. Fill in:
   - **Name**: PENS Email Service
   - **Supported account types**: 
     - For personal accounts (Outlook.com/Hotmail): "Personal Microsoft accounts only"
     - For Microsoft 365: "Accounts in this organizational directory only"
   - **Redirect URI**: Leave blank (we use device code flow)
5. Click **Register**
6. **Save the Application (client) ID** - you'll need this!

### Step 2: Configure API Permissions

1. In your app, go to **API permissions**
2. Click **+ Add a permission**
3. Select **Microsoft Graph** or **Office 365 Exchange Online**
4. Choose **Delegated permissions**
5. Add these permissions:
   - `IMAP.AccessAsUser.All` (for reading emails)
   - `SMTP.Send` (for sending emails)
   - `offline_access` (for refresh tokens)
6. Click **Add permissions**
7. Click **✓ Grant admin consent** (if you're admin)

### Step 3: Get OAuth Access Token

```bash
cd /Users/bf/Dev/HelloWorld/Velivolant/pens

# Set your Azure AD Client ID
export AZURE_CLIENT_ID="your-client-id-from-step-1"

# Run the OAuth helper
node scripts/oauth-token-helper.js
```

**Follow the prompts:**
1. Browser opens to Microsoft login
2. Enter code shown in terminal
3. Sign in with `info@velivolant.io`
4. Grant permissions
5. Token automatically saved!

### Step 4: Update Configuration

Edit `config/pens.conf`:

```ini
# Switch to OAuth authentication
auth_method = oauth

# Comment out or remove password
# imap_password = ...

# Add OAuth tokens (from .oauth_token.json)
oauth_access_token = eyJ0eXAiOiJKV1QiLCJub25jZSI6...
oauth_refresh_token = M.R3_BAY.-CQ...
```

### Step 5: Build and Run

```bash
# Rebuild PENS with OAuth support
make clean && make

# Run with OAuth
./pens -c config/pens.conf
```

Done! PENS now uses OAuth 2.0! 🎉

---

## 📋 Detailed Azure AD Setup

### Create Application Registration

1. **Go to Azure Portal**: https://portal.azure.com
2. **Azure Active Directory** → **App registrations** → **New registration**
3. **Application name**: PENS Email Service for Velivolant
4. **Supported account types**:
   - Personal Microsoft accounts: For @outlook.com, @hotmail.com, @live.com
   - Single tenant: For organization-specific @company.com
   - Multitenant: For multiple organizations
5. **Redirect URI**: Not needed (using device code flow)
6. Click **Register**

### Note Your IDs

After registration, you'll see:
- **Application (client) ID**: `12345678-1234-1234-1234-123456789abc`
- **Directory (tenant) ID**: `common` or `consumers` or your org ID

Save these!

### API Permissions

#### For Microsoft 365 (Work/School):
1. **API permissions** → **Add permission**
2. **APIs my organization uses** → Search "Office 365 Exchange Online"
3. **Delegated permissions** → Add:
   - `IMAP.AccessAsUser.All`
   - `SMTP.Send`
   - `offline_access`

#### For Personal Accounts (Outlook.com):
1. **API permissions** → **Add permission**
2. **Microsoft Graph**
3. **Delegated permissions** → Add:
   - `Mail.Read`
   - `Mail.ReadWrite`
   - `Mail.Send`
   - `offline_access`

#### Grant Consent

- If you're an admin: Click **✓ Grant admin consent for [Your Org]**
- If not: Users will consent when they first authenticate

---

## 🔧 OAuth Token Helper Script

### What It Does

```
1. Starts device code flow
2. Shows you a URL and code
3. You visit URL, enter code, sign in
4. Script receives access token
5. Saves tokens to .oauth_token.json
6. Creates .oauth_env_snippet for easy setup
```

### Usage

```bash
cd /Users/bf/Dev/HelloWorld/Velivolant/pens

# Basic usage (for consumers/personal accounts)
AZURE_CLIENT_ID="your-client-id" node scripts/oauth-token-helper.js

# For work/school accounts
AZURE_CLIENT_ID="your-client-id" \
AZURE_TENANT_ID="your-tenant-id" \
node scripts/oauth-token-helper.js
```

### Output Files

**`.oauth_token.json`** (gitignored):
```json
{
  "access_token": "eyJ0...",
  "refresh_token": "M.R3...",
  "expires_in": 3600,
  "token_type": "Bearer",
  "scope": "...",
  "acquired_at": 1698437000000
}
```

**`.oauth_env_snippet`** (gitignored):
```bash
PENS_AUTH_METHOD=oauth
PENS_OAUTH_ACCESS_TOKEN=eyJ0...
PENS_OAUTH_REFRESH_TOKEN=M.R3...
```

---

## 📊 Authentication Flow

### Device Code Flow

```
┌──────────────┐
│  Run Script  │
└──────┬───────┘
       │
       ▼
┌──────────────────────────────┐
│ Script: Request device code  │
└──────┬───────────────────────┘
       │
       ▼
┌───────────────────────────────────┐
│ Microsoft: Returns device code    │
│ - verification_uri                │
│ - user_code                       │
│ - device_code                     │
└──────┬────────────────────────────┘
       │
       ▼
┌────────────────────────────────────┐
│ User: Opens browser                │
│ - Visits verification_uri          │
│ - Enters user_code                 │
│ - Signs in with info@velivolant.io │
│ - Grants permissions               │
└──────┬─────────────────────────────┘
       │
       ▼
┌───────────────────────────────────┐
│ Script: Polls for token (every 5s)│
└──────┬────────────────────────────┘
       │
       ▼
┌───────────────────────────────────┐
│ Microsoft: Returns tokens         │
│ - access_token (valid ~1 hour)    │
│ - refresh_token (long-lived)      │
└──────┬────────────────────────────┘
       │
       ▼
┌───────────────────────────────────┐
│ Script: Saves tokens to file      │
│ .oauth_token.json                 │
└───────────────────────────────────┘
```

### PENS Authentication Flow

```
┌──────────────┐
│  Start PENS  │
└──────┬───────┘
       │
       ▼
┌──────────────────────────┐
│ Config: auth_method?     │
└──────┬──────────┬────────┘
       │          │
  "password"   "oauth"
       │          │
       ▼          ▼
┌──────────────┐  ┌─────────────────┐
│ AUTH LOGIN   │  │ AUTH XOAUTH2    │
│ (username +  │  │ (access token)  │
│  password)   │  │                 │
└──────┬───────┘  └────────┬────────┘
       │                   │
       └──────────┬────────┘
                  ▼
         ┌────────────────┐
         │  Authenticated │
         │  ✅ Connected  │
         └────────────────┘
```

---

## 🔄 Token Refresh

Access tokens expire after ~1 hour. You have 3 options:

### Option 1: Manual Refresh
Run the oauth helper script again when token expires:
```bash
node scripts/oauth-token-helper.js
```

### Option 2: Automated Refresh (Future)
PENS could automatically refresh tokens:
```cpp
// Check if token expired
if (OAuthHelper::isTokenExpired(tokenTimestamp)) {
    // Use refresh token to get new access token
    newAccessToken = refreshOAuthToken(refreshToken);
}
```

### Option 3: Use Long-Lived App Password
If OAuth is too complex for your use case, stick with app passwords.

---

## 🆘 Troubleshooting

### "Client ID not configured"

Set the environment variable:
```bash
export AZURE_CLIENT_ID="your-app-registration-client-id"
```

Or edit the script and hardcode your client ID.

### "Authentication failed - invalid_client"

- Double-check your client ID is correct
- Ensure app registration hasn't been deleted
- Verify you're using the right tenant ID

### "Authentication failed - insufficient permissions"

1. Go to Azure Portal → Your app → API permissions
2. Ensure these are added:
   - IMAP.AccessAsUser.All
   - SMTP.Send  
   - offline_access
3. Click "Grant admin consent"

### "XOAUTH2 not enabled"

Some email servers don't support XOAUTH2. Check:
- Microsoft 365: ✅ Supported
- Outlook.com personal: ✅ Supported (with app registration)
- Gmail: ✅ Supported
- Other providers: May not support OAuth

### "Access token expired"

Tokens expire after ~1 hour. Either:
- Run `node scripts/oauth-token-helper.js` again
- Implement refresh token logic
- Use app password instead

---

## 🔐 Security Best Practices

### Protect Your Tokens
- ✅ Never commit `.oauth_token.json` to git (already gitignored)
- ✅ Never share access or refresh tokens
- ✅ Rotate tokens regularly
- ✅ Revoke tokens if compromised

### Revoke Tokens

If tokens are compromised:
1. Go to https://account.microsoft.com/privacy/app-access
2. Find "PENS Email Service"
3. Click **Remove access**
4. Generate new tokens

### Monitor Usage

Check app usage:
1. Azure Portal → Your app → **Sign-in logs**
2. Review authentication events
3. Monitor for suspicious activity

---

## 📚 Technical Details

### XOAUTH2 SASL Format

```
base64(user={email}\x01auth=Bearer {token}\x01\x01)
```

Example:
```
user=info@velivolant.io\x01auth=Bearer eyJ0eXA...\x01\x01
↓ base64 encode
dXNlcj1pbmZvQHZlbGl2b2xhbnQuaW8BYXV0aD1CZWFyZXIgZXlKMGVY...
```

### SMTP XOAUTH2 Command

```
AUTH XOAUTH2 dXNlcj1pbmZvQHZlbGl2b2xhbnQuaW8BYXV0aD1CZWFyZXI...
235 2.7.0 Authentication successful
```

### IMAP XOAUTH2 Command

```
A002 AUTHENTICATE XOAUTH2 dXNlcj1pbmZvQHZlbGl2b2xhbnQuaW8BYXV0aD1CZWFyZXI...
A002 OK AUTHENTICATE completed
```

---

## 🌐 Microsoft 365 OAuth Endpoints

### Authorization
```
https://login.microsoftonline.com/{tenant}/oauth2/v2.0/authorize
```

### Token
```
https://login.microsoftonline.com/{tenant}/oauth2/v2.0/token
```

### Device Code
```
https://login.microsoftonline.com/{tenant}/oauth2/v2.0/devicecode
```

### Tenant IDs
- Personal accounts: `consumers`
- Work/school: `common` or your organization's tenant ID
- Specific tenant: `12345678-1234-1234-1234-123456789abc`

---

## ✅ Comparison: App Password vs OAuth

| Feature | App Password | OAuth 2.0 |
|---------|-------------|-----------|
| Security | Good | Better |
| Setup | Easy | Moderate |
| Expiration | Never | Tokens expire (~1hr) |
| Revocation | Delete password | Instant revoke |
| Permissions | Full access | Granular scopes |
| Modern Auth | May not work | Required for Modern Auth |
| Best for | Quick setup | Production use |

---

## 📝 Complete Example

### 1. Register App

```
Azure Portal → Azure AD → App registrations → New
Name: PENS for Velivolant
Type: Personal accounts
→ Register
→ Copy client ID: 12345678-abcd-1234-abcd-123456789abc
```

### 2. Add Permissions

```
App → API permissions → Add permission
→ Office 365 Exchange Online
→ Delegated permissions
→ Add: IMAP.AccessAsUser.All, SMTP.Send, offline_access
→ Grant admin consent
```

### 3. Get Token

```bash
cd /Users/bf/Dev/HelloWorld/Velivolant/pens

AZURE_CLIENT_ID="12345678-abcd-1234-abcd-123456789abc" \
node scripts/oauth-token-helper.js

# Follow prompts:
# - Visit https://microsoft.com/devicelogin
# - Enter code: ABC-DEF-GHI
# - Sign in
# - Grant permissions
```

### 4. Configure PENS

Edit `config/pens.conf`:
```ini
auth_method = oauth
imap_username = info@velivolant.io
oauth_access_token = eyJ0eXAiOiJKV1QiLCJub25jZSI6...
oauth_refresh_token = M.R3_BAY.-CQ...
```

### 5. Run PENS

```bash
make clean && make
./pens -c config/pens.conf
```

---

## 🔄 Token Lifecycle

```
1. Initial Setup
   ├─ Run oauth-token-helper.js
   ├─ Get access_token (1 hour) + refresh_token (long-lived)
   └─ Save to config

2. Daily Use (< 1 hour)
   ├─ PENS uses access_token
   └─ Works perfectly ✅

3. After 1 Hour
   ├─ Access token expires
   ├─ PENS authentication fails ❌
   └─ Need to refresh

4. Refresh Token
   ├─ Run oauth-token-helper.js again
   ├─ Or implement auto-refresh
   └─ Get new access_token

5. Refresh Token Expires (months/years later)
   └─ Re-authenticate from scratch
```

---

## 🔨 Implementation Details

### Files Added/Modified

**New Files:**
- `include/oauth_helper.hpp` - OAuth helper class
- `src/oauth_helper.cpp` - OAuth implementation
- `scripts/oauth-token-helper.js` - Token acquisition script
- `OAUTH_SETUP_GUIDE.md` - This file

**Modified Files:**
- `include/smtp_client.hpp` - Added `authenticateOAuth()` method
- `src/smtp_client.cpp` - Implemented XOAUTH2 for SMTP
- `include/imap_client.hpp` - Added `authenticateOAuth()` method
- `src/imap_client.cpp` - Implemented XOAUTH2 for IMAP
- `include/config.hpp` - Added OAuth config getters
- `src/config.cpp` - OAuth config implementation
- `src/main.cpp` - OAuth/password selection logic
- `config/pens.conf` - OAuth configuration template
- `.gitignore` - Ignore token files

### Code Changes

**OAuth String Generation:**
```cpp
std::string OAuthHelper::generateXOAuth2String(
    const std::string& email,
    const std::string& accessToken
) {
    // Format: user={email}\x01auth=Bearer {token}\x01\x01
    std::ostringstream authString;
    authString << "user=" << email << "\x01"
               << "auth=Bearer " << accessToken << "\x01\x01";
    
    return base64Encode(authString.str());
}
```

**SMTP OAuth Auth:**
```cpp
bool SmtpClient::authenticateOAuth(
    const std::string& username,
    const std::string& accessToken
) {
    std::string xoauth2 = OAuthHelper::generateXOAuth2String(username, accessToken);
    sendCommand("AUTH XOAUTH2 " + xoauth2 + "\r\n");
    return readResponse(235);  // 235 = Authentication successful
}
```

---

## 📖 References

- [Microsoft Identity Platform Docs](https://docs.microsoft.com/en-us/azure/active-directory/develop/)
- [OAuth 2.0 Device Code Flow](https://docs.microsoft.com/en-us/azure/active-directory/develop/v2-oauth2-device-code)
- [XOAUTH2 SASL Mechanism](https://developers.google.com/gmail/imap/xoauth2-protocol)
- [Microsoft Graph API Permissions](https://docs.microsoft.com/en-us/graph/permissions-reference)

---

## ✅ Checklist

### Azure AD Setup
- [ ] Created app registration
- [ ] Noted client ID
- [ ] Added API permissions (IMAP, SMTP, offline_access)
- [ ] Granted admin consent (if needed)

### Token Acquisition  
- [ ] Set AZURE_CLIENT_ID environment variable
- [ ] Ran oauth-token-helper.js
- [ ] Completed browser authentication
- [ ] Received access and refresh tokens
- [ ] Tokens saved to .oauth_token.json

### PENS Configuration
- [ ] Updated config/pens.conf with auth_method=oauth
- [ ] Added oauth_access_token to config
- [ ] Added oauth_refresh_token to config (optional)
- [ ] Removed or commented out password

### Testing
- [ ] Rebuilt PENS: `make clean && make`
- [ ] Ran PENS: `./pens -c config/pens.conf`
- [ ] Verified OAuth authentication successful
- [ ] Tested email sending/receiving
- [ ] Confirmed IMAP and SMTP both work

---

**OAuth 2.0 support is ready! Secure, modern authentication for PENS. 🔐**

