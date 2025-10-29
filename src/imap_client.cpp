#include "imap_client.hpp"
#include "oauth_helper.hpp"
#include "logger.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace Pens {

// Internal connection structure
struct ImapClient::ImapConnection {
    int socket;
    SSL* ssl;
    SSL_CTX* sslContext;
    
    ImapConnection() : socket(-1), ssl(nullptr), sslContext(nullptr) {}
    
    ~ImapConnection() {
        if (ssl) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
        }
        if (sslContext) {
            SSL_CTX_free(sslContext);
        }
        if (socket >= 0) {
            close(socket);
        }
    }
};

ImapClient::ImapClient(const std::string& server, int port, bool useSsl)
    : connection_(std::make_unique<ImapConnection>()),
      server_(server),
      port_(port),
      useSsl_(useSsl),
      connected_(false),
      authenticated_(false),
      currentMailbox_("") {
    
    LOG_INFO("PENS IMAP Client initialized for server: " + server);
}

ImapClient::~ImapClient() {
    if (connected_) {
        disconnect();
    }
}

bool ImapClient::connect() {
    LOG_INFO("Attempting to connect to " + server_ + ":" + std::to_string(port_));
    
    // Resolve hostname
    struct hostent* host = gethostbyname(server_.c_str());
    if (!host) {
        LOG_ERROR("Failed to resolve hostname: " + server_);
        return false;
    }
    
    // Create socket
    connection_->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connection_->socket < 0) {
        LOG_ERROR("Failed to create socket");
        return false;
    }
    
    // Connect
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    std::memcpy(&serverAddr.sin_addr, host->h_addr_list[0], host->h_length);
    
    if (::connect(connection_->socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        LOG_ERROR("Failed to connect to server");
        close(connection_->socket);
        connection_->socket = -1;
        return false;
    }
    
    // Setup SSL if needed
    if (useSsl_) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        
        connection_->sslContext = SSL_CTX_new(TLS_client_method());
        if (!connection_->sslContext) {
            LOG_ERROR("Failed to create SSL context");
            return false;
        }
        
        connection_->ssl = SSL_new(connection_->sslContext);
        SSL_set_fd(connection_->ssl, connection_->socket);
        
        if (SSL_connect(connection_->ssl) != 1) {
            LOG_ERROR("SSL handshake failed");
            return false;
        }
        
        LOG_INFO("SSL connection established");
    }
    
    connected_ = true;
    LOG_INFO("Successfully connected to IMAP server");
    
    // Read welcome message
    std::string welcome = sendCommand("");
    LOG_DEBUG("Server welcome: " + welcome);
    
    return true;
}

bool ImapClient::authenticate(const std::string& username, const std::string& password) {
    if (!connected_) {
        LOG_ERROR("Cannot authenticate: not connected");
        return false;
    }
    
    LOG_INFO("Authenticating as: " + username);
    
    // Send LOGIN command with properly quoted credentials
    // Gmail IMAP requires quoted strings for username and password
    std::string loginCmd = "A001 LOGIN \"" + username + "\" \"" + password + "\"\r\n";
    std::string response = sendCommand(loginCmd);
    
    LOG_DEBUG("IMAP LOGIN response: " + response);
    
    if (response.find("OK") != std::string::npos) {
        authenticated_ = true;
        LOG_INFO("Authentication successful");
        return true;
    }
    
    LOG_ERROR("Authentication failed");
    return false;
}

bool ImapClient::authenticateOAuth(const std::string& username, const std::string& accessToken) {
    if (!connected_) {
        LOG_ERROR("Cannot authenticate: not connected");
        return false;
    }
    
    LOG_INFO("Authenticating with OAuth 2.0 as: " + username);
    
    // Generate XOAUTH2 authentication string
    std::string xoauth2String = OAuthHelper::generateXOAuth2String(username, accessToken);
    
    // AUTHENTICATE XOAUTH2 command
    std::string authCmd = "A002 AUTHENTICATE XOAUTH2 " + xoauth2String + "\r\n";
    std::string response = sendCommand(authCmd);
    
    LOG_DEBUG("IMAP OAUTH response: " + response);
    
    if (response.find("OK") != std::string::npos) {
        authenticated_ = true;
        LOG_INFO("OAuth authentication successful");
        return true;
    }
    
    LOG_ERROR("OAuth authentication failed");
    LOG_ERROR("This may be due to:");
    LOG_ERROR("  - Invalid or expired access token");
    LOG_ERROR("  - Insufficient permissions/scopes (need Mail.Read or similar)");
    LOG_ERROR("  - XOAUTH2 not enabled on the server");
    LOG_ERROR("  - Incorrect tenant configuration");
    return false;
}

bool ImapClient::disconnect() {
    if (!connected_) {
        return true;
    }
    
    if (authenticated_) {
        sendCommand("A999 LOGOUT\r\n");
    }
    
    connection_.reset(new ImapConnection());
    connected_ = false;
    authenticated_ = false;
    
    LOG_INFO("Disconnected from IMAP server");
    
    return true;
}

bool ImapClient::isConnected() const {
    return connected_ && authenticated_;
}

bool ImapClient::selectMailbox(const std::string& mailbox) {
    if (!authenticated_) {
        LOG_ERROR("Cannot select mailbox: not authenticated");
        return false;
    }
    
    std::string cmd = "A002 SELECT " + mailbox + "\r\n";
    std::string response = sendCommand(cmd);
    
    if (response.find("OK") != std::string::npos) {
        currentMailbox_ = mailbox;
        LOG_INFO("Selected mailbox: " + mailbox);
        return true;
    }
    
    LOG_ERROR("Failed to select mailbox: " + mailbox);
    return false;
}

std::vector<std::string> ImapClient::listMailboxes() {
    std::vector<std::string> mailboxes;
    
    if (!authenticated_) {
        LOG_ERROR("Cannot list mailboxes: not authenticated");
        return mailboxes;
    }
    
    std::string cmd = "A003 LIST \"\" \"*\"\r\n";
    std::string response = sendCommand(cmd);
    
    // Parse mailbox list (simplified)
    std::istringstream iss(response);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("* LIST") != std::string::npos) {
            size_t start = line.find_last_of("\"") + 1;
            if (start != std::string::npos) {
                mailboxes.push_back(line.substr(start));
            }
        }
    }
    
    LOG_INFO("Found " + std::to_string(mailboxes.size()) + " mailboxes");
    return mailboxes;
}

int ImapClient::getMessageCount() {
    if (currentMailbox_.empty()) {
        selectMailbox("INBOX");
    }
    
    std::string cmd = "A004 STATUS " + currentMailbox_ + " (MESSAGES)\r\n";
    std::string response = sendCommand(cmd);
    
    // Parse message count (simplified)
    size_t pos = response.find("MESSAGES");
    if (pos != std::string::npos) {
        std::istringstream iss(response.substr(pos + 8));
        int count;
        iss >> count;
        
        LOG_DEBUG("Found " + std::to_string(count) + " emails");
        
        return count;
    }
    
    return 0;
}

std::vector<Email> ImapClient::fetchRecentEmails(int count) {
    std::vector<Email> emails;
    
    if (currentMailbox_.empty()) {
        selectMailbox("INBOX");
    }
    
    LOG_INFO("Fetching " + std::to_string(count) + " recent emails");
    
    // Fetch UIDs
    std::string cmd = "A005 UID SEARCH ALL\r\n";
    std::string response = sendCommand(cmd);
    
    // Parse UIDs (simplified - in reality this would be more robust)
    std::vector<std::string> uids;
    std::istringstream iss(response);
    std::string word;
    bool inSearch = false;
    while (iss >> word) {
        if (word == "SEARCH") {
            inSearch = true;
            continue;
        }
        if (inSearch && std::isdigit(word[0])) {
            uids.push_back(word);
        }
    }
    
    // Fetch recent emails
    int fetchCount = std::min(count, static_cast<int>(uids.size()));
    for (int i = uids.size() - fetchCount; i < static_cast<int>(uids.size()); i++) {
        if (i >= 0) {
            Email email = fetchEmail(uids[i]);
            emails.push_back(email);
        }
    }
    
    LOG_DEBUG("Retrieved " + std::to_string(emails.size()) + " emails");
    
    return emails;
}

Email ImapClient::fetchEmail(const std::string& uid) {
    Email email;
    email.id = uid;
    email.isRead = false;
    
    // Fetch email headers and body
    std::string cmd = "A006 UID FETCH " + uid + " (FLAGS BODY[HEADER] BODY[TEXT])\r\n";
    std::string response = sendCommand(cmd);
    
    email = parseEmailData(response, uid);
    email.priority = calculatePriorityScore(email);
    
    return email;
}

bool ImapClient::markAsRead(const std::string& uid) {
    std::string cmd = "A007 UID STORE " + uid + " +FLAGS (\\Seen)\r\n";
    std::string response = sendCommand(cmd);
    return response.find("OK") != std::string::npos;
}

bool ImapClient::deleteEmail(const std::string& uid) {
    std::string cmd = "A008 UID STORE " + uid + " +FLAGS (\\Deleted)\r\n";
    std::string response = sendCommand(cmd);
    
    if (response.find("OK") != std::string::npos) {
        sendCommand("A009 EXPUNGE\r\n");
        return true;
    }
    return false;
}

std::string ImapClient::getConnectionStatus() const {
    if (connected_ && authenticated_) {
        return "Connected and authenticated to " + server_;
    } else if (connected_) {
        return "Connected but not authenticated";
    }
    return "Not connected";
}

std::string ImapClient::sendCommand(const std::string& command) {
    if (!connected_) {
        return "";
    }
    
    // Send command
    if (!command.empty()) {
        if (useSsl_ && connection_->ssl) {
            SSL_write(connection_->ssl, command.c_str(), command.length());
        } else {
            send(connection_->socket, command.c_str(), command.length(), 0);
        }
    }
    
    // Receive response
    char buffer[4096];
    std::string response;
    int bytes;
    
    do {
        if (useSsl_ && connection_->ssl) {
            bytes = SSL_read(connection_->ssl, buffer, sizeof(buffer) - 1);
        } else {
            bytes = recv(connection_->socket, buffer, sizeof(buffer) - 1, 0);
        }
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            response += buffer;
        }
    } while (bytes == sizeof(buffer) - 1);
    
    return response;
}

bool ImapClient::parseResponse(const std::string& response) {
    return response.find("OK") != std::string::npos;
}

Email ImapClient::parseEmailData(const std::string& data, const std::string& uid) {
    Email email;
    email.id = uid;
    
    // Simplified parsing (in reality, use a proper MIME parser)
    std::istringstream iss(data);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.find("From:") == 0) {
            email.from = line.substr(5);
        } else if (line.find("Subject:") == 0) {
            email.subject = line.substr(8);
        } else if (line.find("Date:") == 0) {
            email.date = line.substr(5);
        }
    }
    
    // Body is everything after headers (simplified)
    size_t bodyPos = data.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        email.body = data.substr(bodyPos + 4);
    }
    
    email.isRead = data.find("\\Seen") != std::string::npos;
    
    return email;
}

int ImapClient::calculatePriorityScore(const Email& email) {
    int score = 5;  // Default medium priority
    
    // Check for urgent keywords
    std::string upperSubject = email.subject;
    std::transform(upperSubject.begin(), upperSubject.end(), 
                   upperSubject.begin(), ::toupper);
    
    if (upperSubject.find("URGENT") != std::string::npos ||
        upperSubject.find("IMPORTANT") != std::string::npos ||
        upperSubject.find("CRITICAL") != std::string::npos) {
        score += 3;
    }
    
    // Check for action items
    if (upperSubject.find("ACTION REQUIRED") != std::string::npos ||
        upperSubject.find("DEADLINE") != std::string::npos) {
        score += 2;
    }
    
    return std::min(10, std::max(1, score));
}

} // namespace Pens

