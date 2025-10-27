# Hotmail/Outlook Authentication Troubleshooting

## 🔴 Current Issue: "LOGIN failed"

You're seeing:
```
[DEBUG] IMAP LOGIN response: A001 NO LOGIN failed.
[ERROR] Authentication failed
```

This means the connection works, but Microsoft Exchange is rejecting your credentials.

---

## ✅ Step-by-Step Fix

### Step 1: Enable IMAP Access (CRITICAL!)

1. Go to: https://outlook.live.com/mail/0/options/mail/accounts
2. Click **"Sync email"**
3. Scroll to **"POP and IMAP"**
4. Make sure **"Let devices and apps use IMAP"** is **ENABLED** ✅
5. Click **Save**

**Note:** If this option is grayed out or doesn't exist, your account might be managed by an organization or has restrictions.

### Step 2: Check Account Type

**Hotmail.com accounts** should work with IMAP if properly configured.

**If your account is:**
- `@hotmail.com` - Should work ✅
- `@outlook.com` - Should work ✅
- `@live.com` - Should work ✅
- Work/School account - Might require OAuth2 ⚠️

### Step 3: Try App Password (If 2FA Enabled)

If you have 2FA enabled, you **must** use an App Password:

1. Go to: https://account.microsoft.com/security
2. Click **"Advanced security options"**
3. Under **"App passwords"**, click **"Create a new app password"**
4. Copy the generated password
5. Use that instead of your regular password

### Step 4: Check Modern Authentication

Microsoft is transitioning to "Modern Authentication" (OAuth2). Some accounts can't use basic authentication anymore.

**Check if Basic Auth is disabled:**
1. Go to: https://account.microsoft.com/security
2. Look for **"Advanced security options"**
3. Check if **"Basic authentication"** is disabled

If it is, you'll need OAuth2 (see below).

---

## 🔍 Quick Diagnostic Commands

### Test 1: Manual IMAP Connection

```bash
openssl s_client -connect outlook.office365.com:993 -crlf
```

Once connected, type:
```
A001 LOGIN "memeGodEmporer@hotmail.com" "Y87p4A4hyuoe*mh2uKfd"
```

**Expected Response:**
- ✅ `A001 OK LOGIN completed` - Credentials work!
- ❌ `A001 NO LOGIN failed` - Credentials rejected (check settings)
- ❌ `A001 BAD Command` - IMAP disabled or OAuth2 required

### Test 2: Check CAPABILITY

```bash
openssl s_client -connect outlook.office365.com:993 -crlf
```

Type:
```
A001 CAPABILITY
```

Look for:
- `AUTH=PLAIN` - Basic auth supported ✅
- `AUTH=XOAUTH2` - OAuth2 required ⚠️
- No `LOGIN` - Basic login disabled ❌

---

## 🛠️ Possible Solutions

### Solution 1: Enable IMAP (Most Likely)

**Problem:** IMAP is disabled on your Hotmail account

**Fix:**
1. https://outlook.live.com/mail/0/options/mail/accounts
2. Enable **"Let devices and apps use IMAP"**
3. Wait 5-10 minutes for changes to propagate
4. Try again

### Solution 2: Use App Password

**Problem:** 2FA is enabled, regular password doesn't work

**Fix:**
1. Enable 2FA if not already: https://account.microsoft.com/security
2. Create App Password: https://account.live.com/proofs/AppPassword
3. Use the app password instead of your regular password
4. Update in launch.json or command line

### Solution 3: Implement OAuth2 (If Required)

**Problem:** Microsoft disabled basic authentication for your account

**Fix:** We need to implement OAuth2 authentication. This requires:

1. Register app in Azure AD
2. Get OAuth2 tokens
3. Use XOAUTH2 SASL mechanism

Let me know if you need this - I can implement OAuth2 support.

### Solution 4: Check Account Security Settings

**Problem:** Account has security restrictions

**Fix:**
1. Go to: https://account.microsoft.com/security
2. Check for:
   - Unusual sign-in activity blocks
   - Country/region restrictions
   - App-specific passwords required
3. Adjust settings or create exceptions

---

## 🔧 Update Configuration

If you get an app password, update your configuration:

### Update launch.json

```json
{
  "name": "PENS_IMAP_PASSWORD",
  "value": "your-16-char-app-password"
}
```

### Update debug-test.sh

```bash
-w "your-16-char-app-password"
```

---

## 📊 Authentication Methods Comparison

| Method | When to Use | How to Enable |
|--------|-------------|---------------|
| **Basic (LOGIN)** | Default, if available | Just use password |
| **App Password** | 2FA enabled | Generate in security settings |
| **OAuth2** | Modern auth required | Requires Azure app registration |

---

## 🧪 Test Current Setup

Run this to see detailed authentication info:

```bash
cd /Users/bf/Dev/HelloWorld/Velivolant/pens

# Test with debug logging
PENS_DEBUG_MODE=true ./pens \
  -s outlook.office365.com \
  -u memeGodEmporer@hotmail.com \
  -w "Y87p4A4hyuoe*mh2uKfd" \
  -o -d 2>&1 | tee auth-debug.log

# Check the log
cat auth-debug.log | grep -i "login\|auth\|error"
```

---

## ❓ Common Issues

### "Login failed" - IMAP Disabled
**Symptoms:** 
```
A001 NO LOGIN failed
```

**Solution:** Enable IMAP in Outlook settings (see Step 1 above)

### "Bad credentials" - Wrong Password
**Symptoms:**
```
A001 NO [AUTHENTICATIONFAILED] Invalid credentials
```

**Solution:** 
- Use app password if 2FA enabled
- Check for typos in password
- Try generating new app password

### "Authentication method not supported"
**Symptoms:**
```
A001 NO Unsupported authentication mechanism
```

**Solution:** Account requires OAuth2 - let me know and I'll implement it

---

## 🚨 Next Steps

**Try these in order:**

1. ✅ **Enable IMAP** in Outlook settings (most common issue!)
   - https://outlook.live.com/mail/0/options/mail/accounts
   - Enable "Let devices and apps use IMAP"
   - Wait 5 minutes

2. ✅ **Try App Password** if you have 2FA
   - https://account.live.com/proofs/AppPassword
   - Create new app password
   - Use in place of regular password

3. ✅ **Test manually** with openssl (see above)
   - Confirms if credentials work
   - Shows exact error message

4. ✅ **Check for security blocks**
   - https://account.microsoft.com/security
   - Look for recent blocks or alerts

5. ⚠️ **If still failing, might need OAuth2**
   - Let me know - I can implement this
   - Requires Azure app registration

---

## 📞 Need OAuth2 Implementation?

If basic authentication is disabled on your account, we need to implement OAuth2. This involves:

1. Creating Azure AD app registration
2. Implementing OAuth2 token flow
3. Using XOAUTH2 SASL mechanism

Let me know if you need this - I can add OAuth2 support to PENS!

---

## 📝 Report Back

Please try enabling IMAP (Step 1) and let me know:

1. Did the IMAP enable option exist?
2. Was it already enabled or did you enable it?
3. After enabling and waiting 5 minutes, does it work?
4. Do you have 2FA enabled?
5. Did you try app password?

This will help me determine the exact issue! 🔍


