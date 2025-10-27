# PENS Debugging - Quick Start

## 🚀 Start Debugging in Cursor (Easiest Method)

1. **Open Run & Debug panel**: Press `⇧⌘D` (Shift+Cmd+D)
2. **Select configuration**: Choose "Debug PENS" from dropdown
3. **Start debugging**: Press `F5` or click green play button

That's it! The debugger will:
- Build with debug symbols automatically
- Load your credentials from environment
- Stop at any breakpoints you set

---

## 🎯 Quick Breakpoint Setup

Click the left margin on these key lines:

### IMAP Authentication
- `src/imap_client.cpp:131` - LOGIN command construction
- `src/imap_client.cpp:136` - Check authentication success

### SMTP Authentication
- `src/smtp_client.cpp:183` - Base64 encode username
- `src/smtp_client.cpp:191` - Base64 encode password
- `src/smtp_client.cpp:193` - Check authentication success

### Main Loop
- `src/main.cpp:150` - Start of main loop
- `src/main.cpp:165` - Email checking logic

---

## ⌨️ Essential Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| **Start/Continue** | `F5` |
| **Stop** | `⇧F5` (Shift+F5) |
| **Step Over** | `F10` |
| **Step Into** | `F11` |
| **Step Out** | `⇧F11` (Shift+F11) |
| **Toggle Breakpoint** | `F9` |

---

## 💻 Command Line Quick Test

### Option 1: Quick Test Script
```bash
cd /Users/bf/Dev/HelloWorld/Velivolant/pens
./scripts/debug-test.sh
# Choose option 1 for quick test
```

### Option 2: Manual Test
```bash
cd /Users/bf/Dev/HelloWorld/Velivolant/pens
make debug
./pens -s imap.gmail.com -u benjidevrel@gmail.com -w "kipcaj-nufqah-rEdj2" -o -d
```

### Option 3: With LLDB
```bash
lldb ./pens
(lldb) breakpoint set --file imap_client.cpp --line 131
(lldb) run -s imap.gmail.com -u benjidevrel@gmail.com -w "kipcaj-nufqah-rEdj2" -o -d
```

---

## 🔍 Debug Configurations Available

### 1. **Debug PENS** (Default - Use This!)
- Uses environment variables for credentials
- Easiest to use
- Best for most debugging

### 2. **Debug PENS (with args)**
- Uses command-line arguments
- Good for testing argument parsing

### 3. **Debug PENS (once and exit)**
- Runs once and exits (no loop)
- Fastest for quick tests
- Uses `-o` flag

### 4. **Debug PENS (attach to process)**
- Attach to already running PENS
- For debugging live issues

---

## 🐛 Common Debugging Tasks

### Check IMAP Authentication

1. Set breakpoint: `src/imap_client.cpp:131`
2. Start debugger (F5)
3. When stopped, in Debug Console type:
   ```
   p loginCmd
   ```
4. Should see: `A001 LOGIN "benjidevrel@gmail.com" "kipcaj-nufqah-rEdj2"`
5. Press F10 to step over
6. Check response:
   ```
   p response
   ```
7. Should contain "OK"

### Check SMTP Base64 Encoding

1. Set breakpoint: `src/smtp_client.cpp:183`
2. Start debugger (F5)
3. When stopped:
   ```
   p username
   p encodedUsername
   ```
4. Verify encoding is correct

### Watch Variable Values

1. Right-click variable → "Add to Watch"
2. Or add in Watch panel manually
3. Values update as you step through code

---

## ⚡ Pro Tips

1. **Use "Debug PENS (once and exit)"** for fastest iteration
2. **Set conditional breakpoints**: Right-click breakpoint → Edit → Add condition
3. **Use Logpoints**: Right-click line number → Add Logpoint (doesn't stop execution)
4. **Check logs**: Tail `pens.log` in another terminal
5. **Clean rebuild**: If weird issues, run `make clean && make debug`

---

## 📊 Verify Debug Build

```bash
# Check if built with debug symbols
file ./pens | grep "not stripped"
# Should show: not stripped

# Check size (debug builds are larger)
ls -lh ./pens
# Debug: ~2-5MB, Release: ~500KB-1MB
```

---

## 🆘 Troubleshooting

### Debugger Won't Start
```bash
# Rebuild debug version
make clean
make debug

# Verify executable
ls -la ./pens
chmod +x ./pens
```

### Breakpoints Not Hitting
```bash
# Make sure you're using debug build
make clean && make debug

# Check if code path is executed
# Add log statement or use Logpoint
```

### Can't See Variables
- Make sure you're at the right stack frame
- Variable might be optimized away in release build
- Use debug build: `make debug`

---

## 📚 Full Documentation

For complete debugging guide, see: [`DEBUG_GUIDE.md`](./DEBUG_GUIDE.md)

---

## 🎬 Example Debugging Session

1. **Set breakpoint** on `src/imap_client.cpp:131`
2. **Press F5** to start debugging
3. **Program stops** at breakpoint
4. **Debug Console**:
   ```
   p username    # Check username
   p password    # Check password
   p loginCmd    # Check full command
   ```
5. **Press F10** to step over `sendCommand()`
6. **Check response**:
   ```
   p response    # Should contain "OK"
   ```
7. **Press F5** to continue

Done! You've debugged IMAP authentication! 🎉

---

## 🔗 Quick Links

- [Full Debug Guide](./DEBUG_GUIDE.md) - Complete debugging documentation
- [VS Code Debugging Docs](https://code.visualstudio.com/docs/cpp/cpp-debug)
- [LLDB Quick Reference](https://lldb.llvm.org/use/map.html)

---

**Happy Debugging! 🐛🔧**


