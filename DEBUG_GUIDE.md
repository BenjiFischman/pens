# PENS Debugging Guide

Complete guide for debugging PENS with GDB/LLDB in Cursor IDE.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Debug Configurations](#debug-configurations)
3. [Debugging Commands](#debugging-commands)
4. [Common Debugging Scenarios](#common-debugging-scenarios)
5. [Troubleshooting](#troubleshooting)

---

## Quick Start

### 1. Build Debug Version

```bash
cd /Users/bf/Dev/HelloWorld/Velivolant/pens
make debug
```

This builds with:
- `-g` flag (debug symbols)
- `-O0` (no optimization)
- `-DDEBUG` (debug macro defined)

### 2. Launch Debugger in Cursor

**Option A: Use Run & Debug Panel**
1. Open the Run & Debug panel (⇧⌘D or Shift+Cmd+D)
2. Select "Debug PENS" from the dropdown
3. Press F5 or click the green play button

**Option B: Use Command Palette**
1. Press ⇧⌘P (Shift+Cmd+P)
2. Type "Debug: Select and Start Debugging"
3. Choose "Debug PENS"

**Option C: Use Keyboard Shortcut**
1. Press F5 directly

---

## Debug Configurations

### Configuration 1: Debug PENS (Environment Variables)

Uses environment variables for credentials:

```json
{
  "name": "Debug PENS",
  "environment": [
    { "name": "PENS_IMAP_USERNAME", "value": "benjidevrel@gmail.com" },
    { "name": "PENS_IMAP_PASSWORD", "value": "kipcaj-nufqah-rEdj2" }
  ]
}
```

**Usage:**
- Easiest for most debugging
- Credentials stored in launch.json
- No command-line arguments needed

### Configuration 2: Debug PENS (with args)

Uses command-line arguments:

```json
{
  "name": "Debug PENS (with args)",
  "args": ["-s", "imap.gmail.com", "-u", "benjidevrel@gmail.com", "-w", "password", "-d"]
}
```

**Usage:**
- Tests command-line parsing
- Easier to change arguments
- Good for testing different configurations

### Configuration 3: Debug PENS (once and exit)

Runs once and exits (doesn't loop):

```json
{
  "name": "Debug PENS (once and exit)",
  "args": ["-o", "-d"]
}
```

**Usage:**
- Quick testing
- Faster debugging cycles
- Uses `-o` flag to run once

### Configuration 4: Debug PENS (attach to process)

Attaches to already running process:

**Usage:**
1. Start PENS manually: `./pens -u user@gmail.com -w password`
2. Select "Debug PENS (attach to process)"
3. Choose the running process from the list

**When to use:**
- Debugging live issues
- Process already started
- Need to debug without restarting

---

## Debugging Commands

### Breakpoints

**Set Breakpoint:**
- Click left margin next to line number
- Or press F9 on a line
- Or use command: `breakpoint set --file smtp_client.cpp --line 183`

**Common Breakpoint Locations:**

```cpp
// IMAP Authentication
src/imap_client.cpp:131   // LOGIN command
src/imap_client.cpp:136   // Success check

// SMTP Authentication  
src/smtp_client.cpp:183   // Base64 username encoding
src/smtp_client.cpp:191   // Base64 password encoding

// Main loop
src/main.cpp:150          // Main processing loop
src/main.cpp:165          // Email checking

// Email processing
src/notification_processor.cpp:50  // Process email
```

### Stepping Through Code

- **F10** (Step Over) - Execute current line, don't enter functions
- **F11** (Step Into) - Enter function calls
- **Shift+F11** (Step Out) - Exit current function
- **F5** (Continue) - Resume execution until next breakpoint

### Inspecting Variables

**In Debug Console:**

```lldb
# Print variable
p username
p password
p response

# Print with formatting
p/x statusCode        # hexadecimal
p/t isConnected       # binary
p/c buffer[0]         # character

# Print structure
p *connection_
p load_metrics

# Print array
p buffer[0]@10        # first 10 elements
```

**Watch Window:**
- Right-click variable → "Add to Watch"
- Or add manually in Watch panel

### Call Stack

View in "Call Stack" panel:
```
main() at main.cpp:150
ImapClient::authenticate() at imap_client.cpp:131
ImapClient::sendCommand() at imap_client.cpp:317
SSL_write() [external]
```

### Conditional Breakpoints

Right-click breakpoint → Edit Breakpoint → Add Condition:

```cpp
// Break only when username equals specific value
username == "benjidevrel@gmail.com"

// Break only on errors
statusCode >= 400

// Break after N iterations
loopCounter == 5
```

---

## Common Debugging Scenarios

### Scenario 1: Debug IMAP Authentication Failure

**Steps:**

1. Set breakpoint at `src/imap_client.cpp:131`
2. Start debugger (F5)
3. When breakpoint hits:
   ```lldb
   # Check the login command being sent
   p loginCmd
   # Expected: A001 LOGIN "benjidevrel@gmail.com" "kipcaj-nufqah-rEdj2"
   
   # Step over (F10) to execute sendCommand
   # Then check response
   p response
   ```

4. Look for:
   - Is `loginCmd` properly formatted with quotes?
   - Does `response` contain "OK" or an error?
   - Is the password correct?

### Scenario 2: Debug SMTP Base64 Encoding

**Steps:**

1. Set breakpoint at `src/smtp_client.cpp:183`
2. Start debugger
3. When breakpoint hits:
   ```lldb
   # Check original username
   p username
   
   # Step over base64Encode call
   # Check encoded result
   p encodedUsername
   
   # Verify encoding is correct
   # For "benjidevrel@gmail.com" should be:
   # "YmVuamlkZXZyZWxAZ21haWwuY29t"
   ```

### Scenario 3: Debug Email Processing Loop

**Steps:**

1. Set breakpoint at `src/main.cpp:165`
2. Add watch for `emailCount`
3. Start debugger
4. Use conditional breakpoint: `emailCount > 0`
5. Step through email processing logic

### Scenario 4: Inspect SSL Connection

**Steps:**

1. Set breakpoint at `src/imap_client.cpp:103` (after SSL_connect)
2. Start debugger
3. Check SSL state:
   ```lldb
   p connection_->ssl
   p useSsl_
   p connected_
   ```

### Scenario 5: Debug Command Response Parsing

**Steps:**

1. Set breakpoint at `src/smtp_client.cpp:285` (readResponse)
2. Start debugger
3. Inspect:
   ```lldb
   # Check buffer contents
   p buffer
   
   # Check expected code
   p expectedCode
   
   # Check parsed code
   p code
   ```

---

## Advanced Debugging

### Memory Inspection

```lldb
# View memory at address
memory read 0x7fff5fbff000

# View as string
memory read --format s 0x7fff5fbff000

# View 100 bytes as hex
memory read --size 1 --count 100 --format x 0x7fff5fbff000
```

### Thread Inspection

```lldb
# List all threads
thread list

# Switch to thread
thread select 2

# Show thread backtrace
thread backtrace
```

### Expression Evaluation

```lldb
# Call function
expr sendCommand("NOOP\r\n")

# Modify variable
expr username = "test@test.com"

# Create new variable
expr std::string testStr = "hello"
```

---

## Debugging Best Practices

### 1. Start with Minimal Test

Use "Debug PENS (once and exit)" configuration:
```bash
# Tests one iteration without looping
./pens -o -d -u benjidevrel@gmail.com -w password
```

### 2. Enable Debug Logging

Set environment variable or use `-d` flag:
```bash
export PENS_DEBUG_MODE=true
export PENS_LOG_LEVEL=DEBUG
```

### 3. Use Logpoints Instead of Print Statements

Right-click line → Add Logpoint:
```
Username: {username}, Response: {response}
```

### 4. Save Breakpoint Sessions

Breakpoints are saved in workspace, so you can:
- Set breakpoints for common issues
- Share debugging setup with team
- Keep breakpoints between sessions

---

## Troubleshooting

### Problem: Debugger Won't Start

**Solution 1: Check LLDB Installation**
```bash
lldb --version
# Should show: lldb-1400.x.x or similar
```

**Solution 2: Rebuild with Debug Symbols**
```bash
make clean
make debug
```

**Solution 3: Check File Permissions**
```bash
chmod +x ./pens
```

### Problem: Breakpoints Not Hitting

**Causes:**
1. Code not compiled with `-g` flag
2. Optimizer removing code (`-O3`)
3. Code path not executed

**Solution:**
```bash
# Verify debug build
file ./pens | grep "not stripped"
# Should show: not stripped

# Check compilation flags
make clean
make debug
```

### Problem: Can't See Variable Values

**Causes:**
1. Variable optimized away
2. Out of scope
3. Not yet initialized

**Solution:**
```lldb
# Try different formats
p variable
p &variable
p (char*)variable

# Check if in scope
frame variable
```

### Problem: "Unable to Start Debugging"

**Solution:**
1. Check `launch.json` has correct `program` path
2. Ensure file exists: `ls -la ./pens`
3. Check MIMode is correct for your OS:
   - macOS: `"MIMode": "lldb"`
   - Linux: `"MIMode": "gdb"`

---

## Command-Line Debugging (Without IDE)

### Using LLDB Directly

```bash
# Start debugger
lldb ./pens

# Set arguments
(lldb) settings set target.run-args -u benjidevrel@gmail.com -w password -d

# Set breakpoint
(lldb) breakpoint set --file imap_client.cpp --line 131
(lldb) breakpoint set --name authenticate

# Run
(lldb) run

# When breakpoint hits
(lldb) print loginCmd
(lldb) next
(lldb) step
(lldb) continue

# Exit
(lldb) quit
```

### Using GDB (Linux)

```bash
# Start debugger
gdb ./pens

# Set arguments
(gdb) set args -u benjidevrel@gmail.com -w password -d

# Set breakpoint
(gdb) break imap_client.cpp:131
(gdb) break ImapClient::authenticate

# Run
(gdb) run

# When breakpoint hits
(gdb) print loginCmd
(gdb) next
(gdb) step
(gdb) continue

# Exit
(gdb) quit
```

---

## Testing Checklist

Use this checklist when debugging PENS:

- [ ] Build with debug symbols (`make debug`)
- [ ] Set breakpoints at key locations
- [ ] Start debugger (F5)
- [ ] Verify IMAP connection established
- [ ] Check IMAP authentication (line 131)
- [ ] Verify SMTP connection
- [ ] Check SMTP authentication (lines 183, 191)
- [ ] Test email fetching
- [ ] Test email sending
- [ ] Check error handling paths
- [ ] Verify cleanup/disconnect

---

## Quick Reference Card

| Action | Shortcut | LLDB Command |
|--------|----------|--------------|
| Start Debug | F5 | `run` |
| Stop Debug | Shift+F5 | `quit` |
| Restart | Cmd+Shift+F5 | `run` |
| Toggle Breakpoint | F9 | `breakpoint set` |
| Step Over | F10 | `next` |
| Step Into | F11 | `step` |
| Step Out | Shift+F11 | `finish` |
| Continue | F5 | `continue` |
| Print Variable | - | `p variable` |
| List Breakpoints | - | `breakpoint list` |

---

## Additional Resources

- [LLDB Quick Start Guide](https://lldb.llvm.org/use/map.html)
- [VS Code C++ Debugging](https://code.visualstudio.com/docs/cpp/cpp-debug)
- [GDB to LLDB Command Map](https://lldb.llvm.org/use/map.html)

---

## Need Help?

If you're stuck:

1. Check the logs in `pens.log`
2. Run with debug flag: `./pens -d`
3. Try command-line debugging first
4. Review this guide's Common Scenarios section
5. Check breakpoints are at correct lines after rebuild


