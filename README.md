# PENS - Professional Email Notification System

A C++-based IMAP client for professional email monitoring and intelligent notification delivery.

## Overview

PENS (Professional Email Notification System) is a production-ready email notification system that connects to IMAP servers, monitors mailboxes, and provides intelligent email prioritization and spam detection.

## Features

- **IMAP Protocol Support**: Full RFC 3501 compliant IMAP client
- **SSL/TLS Encryption**: Secure connections with OpenSSL
- **Priority Classification**: Intelligent email priority scoring (1-10)
- **Spam Detection**: Automated spam filtering with configurable thresholds
- **Email Categorization**: Automatic categorization (Meeting, Financial, Newsletter, etc.)
- **Keyword Extraction**: Extract important keywords from email subjects
- **Batch Processing**: Efficient batch email processing with summaries
- **Docker Support**: Full containerization with Docker and Docker Compose
- **Configurable**: Multiple configuration sources (CLI, env vars, config files)

## Quick Start

### Prerequisites

- C++ compiler with C++17 support (g++)
- OpenSSL development libraries
- make
- Docker (optional)

### Build from Source

```bash
# Build release version
make release

# Run with your credentials
./pens -u your-email@gmail.com -w your-app-password
```

### Docker

```bash
# Create environment file
cp env.example .env
# Edit .env with your credentials

# Run with Docker Compose
docker-compose up -d

# View logs
docker-compose logs -f
```

## Configuration

### Environment Variables

```bash
PENS_IMAP_SERVER=imap.gmail.com
PENS_IMAP_PORT=993
PENS_IMAP_USERNAME=your-email@gmail.com
PENS_IMAP_PASSWORD=your-app-password
PENS_IMAP_USE_SSL=true
PENS_PRIORITY_THRESHOLD=5
PENS_CHECK_INTERVAL=60
```

### Command Line Options

```
Usage: pens [options]

Options:
  -h, --help              Show help message
  -c, --config FILE       Load configuration from FILE
  -s, --server SERVER     IMAP server address
  -p, --port PORT         IMAP port (default: 993)
  -u, --username USER     IMAP username
  -w, --password PASS     IMAP password
  -t, --threshold LEVEL   Priority threshold (1-10, default: 5)
  -i, --interval SECONDS  Check interval (default: 60)
  -d, --debug             Enable debug mode
  -o, --once              Process once and exit
```

## Email Provider Support

PENS works with any IMAP-compatible email provider:

- Gmail (imap.gmail.com:993)
- Outlook (imap-mail.outlook.com:993)
- Yahoo (imap.mail.yahoo.com:993)
- iCloud (imap.mail.me.com:993)
- Custom IMAP servers

### Gmail Setup

1. Enable 2-Factor Authentication
2. Generate App Password at https://myaccount.google.com/apppasswords
3. Use the App Password (not your regular password)

## Architecture

PENS consists of several key components:

- **ImapClient**: Handles IMAP protocol communication and SSL/TLS
- **NotificationProcessor**: Analyzes emails and generates notifications
- **PensManager**: Orchestrates email monitoring and processing
- **Config**: Manages configuration from multiple sources
- **Logger**: Thread-safe logging system

## Building

```bash
# Check dependencies
make check-deps

# Debug build
make debug

# Release build (optimized)
make release

# Install system-wide
sudo make install

# Clean build artifacts
make clean
```

## Docker Deployment

```bash
# Build image
make docker-build

# Run container
make docker-run

# View logs
make docker-logs

# Stop container
make docker-stop
```

## Email Priority System

PENS assigns priority scores (1-10) based on:

- Urgent keywords (URGENT, IMPORTANT, CRITICAL)
- Action items (ACTION REQUIRED, DEADLINE)
- Spam indicators (reduces priority)

## Spam Detection

Spam scoring is based on:

- Common spam keywords
- Excessive punctuation
- ALL CAPS subject lines
- Suspicious sender patterns

Configurable spam threshold (default: 70/100)

## Development

### Project Structure

```
pens/
├── include/           # Header files
├── src/              # Implementation files
├── config/           # Configuration templates
├── scripts/          # Helper scripts
├── Dockerfile        # Container definition
├── docker-compose.yml # Orchestration
└── Makefile          # Build system
```

### Code Style

- C++17 standard
- RAII for resource management
- Smart pointers (unique_ptr, shared_ptr)
- Thread-safe logging
- Comprehensive error handling

## Testing

```bash
# Run application once
./pens -u user@email.com -w password --once

# Enable debug logging
./pens -u user@email.com -w password -d

# Custom check interval (30 seconds)
./pens -u user@email.com -w password -i 30
```

## Performance

- Startup Time: ~1 second
- Connection: ~500ms (with SSL)
- Email Fetch: ~100-200ms per email
- Processing: <5ms per email
- Memory Usage: ~10-20MB
- CPU Usage: Minimal (I/O bound)

## Security

- SSL/TLS encryption by default
- Credentials never logged
- Non-root container execution
- Input validation
- Certificate verification

## License

MIT License

## Technical Details

- **Language**: C++17
- **SSL/TLS**: OpenSSL
- **Protocol**: IMAP4rev1 (RFC 3501)
- **Build System**: Make
- **Container**: Docker

## Troubleshooting

**Connection Failed**
- Verify server address and port
- Check SSL is enabled
- Ensure firewall allows port 993

**Authentication Failed**
- Use App Password for Gmail (not regular password)
- Verify IMAP is enabled in email settings
- Check username format (usually full email address)

**SSL Errors**
- Update OpenSSL: `brew upgrade openssl` (macOS) or `apt upgrade openssl` (Linux)

---

**PENS - Professional Email Notification System**  
Making email monitoring efficient and intelligent.
