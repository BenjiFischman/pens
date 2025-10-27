#include "smtp_client.hpp"
#include "logger.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <ctime>

namespace Pens {

// Base64 encoding helper function
static std::string base64Encode(const std::string& input) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    
    BIO_write(bio, input.c_str(), input.length());
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    
    return result;
}

// Internal connection structure
struct SmtpClient::SmtpConnection {
    int socket;
    SSL* ssl;
    SSL_CTX* sslContext;
    
    SmtpConnection() : socket(-1), ssl(nullptr), sslContext(nullptr) {}
    
    ~SmtpConnection() {
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

SmtpClient::SmtpClient(const std::string& server, int port, bool useSsl)
    : connection_(std::make_unique<SmtpConnection>()),
      server_(server),
      port_(port),
      useSsl_(useSsl),
      connected_(false),
      authenticated_(false),
      username_("") {
    
    LOG_INFO("PENS SMTP Client initialized for server: " + server);
}

SmtpClient::~SmtpClient() {
    if (connected_) {
        disconnect();
    }
}

bool SmtpClient::connect() {
    LOG_INFO("Attempting to connect to SMTP " + server_ + ":" + std::to_string(port_));
    
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
        LOG_ERROR("Failed to connect to SMTP server");
        close(connection_->socket);
        connection_->socket = -1;
        return false;
    }
    
    // Read welcome message
    if (!readResponse(220)) {
        LOG_ERROR("SMTP server did not send welcome message");
        return false;
    }
    
    // Say EHLO
    std::string ehlo = "EHLO localhost\r\n";
    sendCommand(ehlo);
    if (!readResponse(250)) {
        LOG_ERROR("EHLO command failed");
        return false;
    }
    
    // Setup SSL/TLS if needed (STARTTLS)
    if (useSsl_ && port_ != 465) {  // 465 = implicit SSL, others use STARTTLS
        sendCommand("STARTTLS\r\n");
        if (!readResponse(220)) {
            LOG_ERROR("STARTTLS command failed");
            return false;
        }
        
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
        
        // Re-send EHLO after STARTTLS
        sendCommand("EHLO localhost\r\n");
        if (!readResponse(250)) {
            LOG_ERROR("EHLO after STARTTLS failed");
            return false;
        }
    }
    
    connected_ = true;
    LOG_INFO("Successfully connected to SMTP server");
    
    return true;
}

bool SmtpClient::authenticate(const std::string& username, const std::string& password) {
    if (!connected_) {
        LOG_ERROR("Cannot authenticate: not connected");
        return false;
    }
    
    LOG_INFO("Authenticating as: " + username);
    username_ = username;
    
    // AUTH LOGIN
    sendCommand("AUTH LOGIN\r\n");
    if (!readResponse(334)) {
        LOG_ERROR("AUTH LOGIN command failed");
        return false;
    }
    
    // Send base64 encoded username
    std::string encodedUsername = base64Encode(username);
    sendCommand(encodedUsername + "\r\n");
    if (!readResponse(334)) {
        LOG_ERROR("Username authentication failed");
        return false;
    }
    
    // Send base64 encoded password
    std::string encodedPassword = base64Encode(password);
    sendCommand(encodedPassword + "\r\n");
    if (!readResponse(235)) {
        LOG_ERROR("Password authentication failed");
        return false;
    }
    
    authenticated_ = true;
    LOG_INFO("SMTP authentication successful");
    
    return true;
}

bool SmtpClient::disconnect() {
    if (!connected_) {
        return true;
    }
    
    if (authenticated_) {
        sendCommand("QUIT\r\n");
        readResponse(221);
    }
    
    connection_.reset(new SmtpConnection());
    connected_ = false;
    authenticated_ = false;
    
    LOG_INFO("Disconnected from SMTP server");
    
    return true;
}

bool SmtpClient::isConnected() const {
    return connected_ && authenticated_;
}

bool SmtpClient::sendEmail(const std::string& from,
                          const std::string& to,
                          const std::string& subject,
                          const std::string& body) {
    if (!authenticated_) {
        LOG_ERROR("Cannot send email: not authenticated");
        return false;
    }
    
    LOG_INFO("Sending email to: " + to);
    
    // MAIL FROM
    sendCommand("MAIL FROM:<" + from + ">\r\n");
    if (!readResponse(250)) {
        LOG_ERROR("MAIL FROM command failed");
        return false;
    }
    
    // RCPT TO
    sendCommand("RCPT TO:<" + to + ">\r\n");
    if (!readResponse(250)) {
        LOG_ERROR("RCPT TO command failed");
        return false;
    }
    
    // DATA
    sendCommand("DATA\r\n");
    if (!readResponse(354)) {
        LOG_ERROR("DATA command failed");
        return false;
    }
    
    // Send email content
    std::string email = formatEmail(from, to, subject, body);
    sendCommand(email);
    sendCommand("\r\n.\r\n");  // End of data
    if (!readResponse(250)) {
        LOG_ERROR("Email data transmission failed");
        return false;
    }
    
    LOG_INFO("Email sent successfully to: " + to);
    
    return true;
}

bool SmtpClient::sendVerificationCode(const std::string& to, const std::string& code) {
    std::ostringstream body;
    body << "Your verification code is: " << code << "\n\n";
    body << "This code will expire in 10 minutes.\n\n";
    body << "If you did not request this code, please ignore this email.\n\n";
    body << "Best regards,\n";
    body << "Velivolant Team";
    
    return sendEmail(username_, to, "Your Verification Code - Velivolant", body.str());
}

std::string SmtpClient::getConnectionStatus() const {
    if (connected_ && authenticated_) {
        return "Connected and authenticated to " + server_;
    } else if (connected_) {
        return "Connected but not authenticated";
    }
    return "Not connected";
}

std::string SmtpClient::sendCommand(const std::string& command) {
    if (!connected_) {
        return "";
    }
    
    // Send command
    if (useSsl_ && connection_->ssl) {
        SSL_write(connection_->ssl, command.c_str(), command.length());
    } else {
        send(connection_->socket, command.c_str(), command.length(), 0);
    }
    
    return "";
}

bool SmtpClient::readResponse(int expectedCode) {
    char buffer[1024];
    int bytes;
    
    if (useSsl_ && connection_->ssl) {
        bytes = SSL_read(connection_->ssl, buffer, sizeof(buffer) - 1);
    } else {
        bytes = recv(connection_->socket, buffer, sizeof(buffer) - 1, 0);
    }
    
    if (bytes <= 0) {
        return false;
    }
    
    buffer[bytes] = '\0';
    std::string response(buffer);
    
    LOG_DEBUG("SMTP response: " + response);
    
    // Parse response code (first 3 characters)
    if (response.length() >= 3) {
        int code = std::stoi(response.substr(0, 3));
        return code == expectedCode;
    }
    
    return false;
}

std::string SmtpClient::formatEmail(const std::string& from,
                                   const std::string& to,
                                   const std::string& subject,
                                   const std::string& body) {
    std::ostringstream email;
    
    // Get current date/time
    std::time_t now = std::time(nullptr);
    char dateStr[100];
    std::strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y %H:%M:%S %z", std::localtime(&now));
    
    email << "From: " << from << "\r\n";
    email << "To: " << to << "\r\n";
    email << "Subject: " << subject << "\r\n";
    email << "Date: " << dateStr << "\r\n";
    email << "Content-Type: text/plain; charset=UTF-8\r\n";
    email << "\r\n";
    email << body;
    
    return email.str();
}

} // namespace Pens

