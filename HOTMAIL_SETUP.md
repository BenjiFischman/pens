# Hotmail/Outlook IMAP Setup Guide

Configuration guide for using PENS with Hotmail/Outlook.com email accounts.

## 📧 Email Provider: Hotmail/Outlook.com

Your configuration has been updated to use:
- **Username**: `memeGodEmporer@hotmail.com`
- **IMAP Server**: `outlook.office365.com`
- **SMTP Server**: `smtp.office365.com`

---

## 🔐 Important: Enable IMAP and Get App Password

### Step 1: Enable IMAP Access

1. Go to [Outlook.com Settings](https://outlook.live.com/mail/0/options/mail/accounts)
2. Click **Sync email** (under "Your app preferences")
3. Enable **"Let devices and apps use POP"** ✅
4. Click **Save**

**Note:** Hotmail/Outlook.com uses the same setting for both POP and IMAP.

### Step 2: Enable 2-Factor Authentication (Required!)

Before you can create an app password, you **must** enable 2FA:

1. Go to [Microsoft Account Security](https://account.microsoft.com/security)
2. Click **Advanced security options**
3. Under **Additional security**, enable **Two-step verification** ✅
4. Follow the setup wizard (use phone or authenticator app)

### Step 3: Create App Password

1. Go to [App Passwords](https://account.live.com/proofs/AppPassword)
2. Click **Create a new app password**
3. Copy the generated password (e.g., `abcd-efgh-ijkl-mnop`)
4. **Save it securely** - you won't see it again!

---

## ⚙️ PENS Configuration

### Server Settings

**IMAP Settings:**
```
Server: outlook.office365.com
Port: 993
Security: SSL/TLS
```

**SMTP Settings:**
```
Server: smtp.office365.com
Port: 587 (STARTTLS)
   or: 465 (SSL/TLS)
Security: STARTTLS or SSL/TLS
```

### Configuration Methods

#### Method 1: Environment Variables (Recommended)

Create a `.env` file or set in your shell:

```bash
export PENS_IMAP_SERVER="outlook.office365.com"
export PENS_IMAP_PORT="993"
export PENS_IMAP_USERNAME="memeGodEmporer@hotmail.com"
export PENS_IMAP_PASSWORD="your-app-password-here"
export PENS_IMAP_USE_SSL="true"

export PENS_SMTP_SERVER="smtp.office365.com"
export PENS_SMTP_PORT="587"
export PENS_SMTP_USE_SSL="true"

export PENS_DEBUG_MODE="true"
export PENS_LOG_LEVEL="DEBUG"
```

#### Method 2: Command Line Arguments

```bash
./pens \
  -s outlook.office365.com \
  -u memeGodEmporer@hotmail.com \
  -w "your-app-password-here" \
  -d
```

#### Method 3: Update launch.json (For Debugging)

The debug configurations are already updated! Just replace `YOUR_PASSWORD_HERE` with your actual app password in `.vscode/launch.json`:

```json
{
  "name": "PENS_IMAP_PASSWORD",
  "value": "abcd-efgh-ijkl-mnop"
}
```

---

## 🚀 Quick Test

### Test IMAP Connection

```bash
cd /Users/bf/Dev/HelloWorld/Velivolant/pens

# Run once and exit
./pens \
  -s outlook.office365.com \
  -u memeGodEmporer@hotmail.com \
  -w "your-app-password-here" \
  -o \
  -d
```

**Expected Output:**
```
[INFO] Starting Professional Email Notification System (PENS)
[INFO] Connecting to IMAP server...
[INFO] Attempting to connect to outlook.office365.com:993
[INFO] SSL connection established
[INFO] Authenticating as: memeGodEmporer@hotmail.com
[INFO] Authentication successful ✅
```

### Test SMTP Connection (Send Email)

```bash
# Test sending verification code
./pens \
  -s outlook.office365.com \
  -u memeGodEmporer@hotmail.com \
  -w "your-app-password-here" \
  --test-smtp \
  -d
```

---

## 🐛 Debugging

The debug configurations are already set up for Hotmail/Outlook:

1. **Update password** in `.vscode/launch.json`
2. Press `⇧⌘D` to open Run & Debug
3. Select **"Debug PENS"**
4. Press `F5` to start debugging

Set breakpoints at:
- `src/imap_client.cpp:131` - IMAP authentication
- `src/smtp_client.cpp:183` - SMTP authentication

---

## 🔧 Troubleshooting

### Problem: "Authentication failed"

**Possible Causes:**
1. IMAP not enabled in Outlook settings
2. Using regular password instead of app password
3. 2FA not enabled
4. Incorrect app password

**Solution:**
```bash
# 1. Verify IMAP is enabled (see Step 1 above)
# 2. Make sure you're using APP PASSWORD, not regular password
# 3. Enable 2FA (see Step 2 above)
# 4. Generate new app password (see Step 3 above)

# Test with verbose logging:
./pens -s outlook.office365.com \
       -u memeGodEmporer@hotmail.com \
       -w "app-password" \
       -o -d 2>&1 | tee test.log
```

### Problem: "Connection refused" or "Timeout"

**Possible Causes:**
1. Firewall blocking port 993
2. Incorrect server address
3. Network issues

**Solution:**
```bash
# Test connection manually
openssl s_client -connect outlook.office365.com:993

# Should see:
# * OK The Microsoft Exchange IMAP4 service is ready

# If this works, PENS should work too
```

### Problem: "SSL handshake failed"

**Solution:**
```bash
# Update OpenSSL (macOS)
brew update
brew upgrade openssl

# Rebuild PENS
cd /Users/bf/Dev/HelloWorld/Velivolant/pens
make clean
make debug
```

---

## 📊 Differences from Gmail

| Feature | Gmail | Hotmail/Outlook |
|---------|-------|-----------------|
| **IMAP Server** | imap.gmail.com | outlook.office365.com |
| **SMTP Server** | smtp.gmail.com | smtp.office365.com |
| **IMAP Port** | 993 (SSL) | 993 (SSL) |
| **SMTP Port** | 587 (STARTTLS) | 587 (STARTTLS) |
| **Auth Method** | App Password | App Password |
| **2FA Required** | Yes | Yes |
| **Folder Structure** | [Gmail]/... | INBOX, Sent, ... |

---

## 📝 Notes for Hotmail/Outlook

### 1. Folder Names

Hotmail/Outlook uses standard IMAP folder names:
- `INBOX` (not `[Gmail]/All Mail`)
- `Sent` (not `[Gmail]/Sent Mail`)
- `Drafts` (not `[Gmail]/Drafts`)
- `Junk` (not `[Gmail]/Spam`)
- `Trash` (not `[Gmail]/Trash`)

### 2. Rate Limiting

Outlook.com has stricter rate limits than Gmail:
- **IMAP**: ~30 connections/hour per account
- **SMTP**: ~300 emails/day for free accounts

Adjust check interval if needed:
```bash
export PENS_CHECK_INTERVAL="120"  # Check every 2 minutes instead of 1
```

### 3. App Password Format

Outlook app passwords are displayed as:
```
abcd-efgh-ijkl-mnop
```

You can use it with or without dashes:
```bash
# Both work:
-w "abcd-efgh-ijkl-mnop"
-w "abcdefghijklmnop"
```

---

## ✅ Verification Checklist

Before running PENS:

- [ ] IMAP enabled in Outlook.com settings
- [ ] 2FA enabled on Microsoft account
- [ ] App password generated
- [ ] App password saved securely
- [ ] `.vscode/launch.json` updated with password
- [ ] Firewall allows port 993 (IMAP)
- [ ] Firewall allows port 587 (SMTP)

---

## 🔗 Useful Links

- [Outlook IMAP Settings](https://support.microsoft.com/en-us/office/pop-imap-and-smtp-settings-for-outlook-com-d088b986-291d-42b8-9564-9c414e2aa040)
- [Enable 2FA](https://account.microsoft.com/security)
- [Create App Password](https://account.live.com/proofs/AppPassword)
- [Microsoft Account Help](https://support.microsoft.com/account-billing)

---

## 🚀 Quick Start Command

Once you have your app password, run:

```bash
cd /Users/bf/Dev/HelloWorld/Velivolant/pens

# Test once
./pens -s outlook.office365.com \
       -u memeGodEmporer@hotmail.com \
       -w "your-app-password" \
       -o -d

# If successful, run continuously
./pens -s outlook.office365.com \
       -u memeGodEmporer@hotmail.com \
       -w "your-app-password" \
       -d
```

---

## 📧 Need Help?

If you're still having issues:

1. Check `pens.log` for detailed error messages
2. Run with debug flag: `-d`
3. Test IMAP manually: `openssl s_client -connect outlook.office365.com:993`
4. Verify app password is correct
5. Check Microsoft account security settings

---

**Your PENS is now configured for Hotmail/Outlook!** 🎉

Just update the password in `.vscode/launch.json` or use command-line arguments, and you're ready to go!


