#ifndef SMTP_CLIENT_HPP
#define SMTP_CLIENT_HPP

#include <string>
#include <memory>

namespace Pens {

/**
 * @brief SMTP Client for sending emails
 * 
 * Simple SMTP client for sending verification code emails
 */
class SmtpClient {
public:
    SmtpClient(const std::string& server, int port, bool useSsl = true);
    ~SmtpClient();
    
    // Connection management
    bool connect();
    bool authenticate(const std::string& username, const std::string& password);
    bool disconnect();
    bool isConnected() const;
    
    // Email sending
    bool sendEmail(const std::string& from,
                   const std::string& to,
                   const std::string& subject,
                   const std::string& body);
    
    bool sendVerificationCode(const std::string& to,
                             const std::string& code);
    
    std::string getConnectionStatus() const;
    
private:
    struct SmtpConnection;
    std::unique_ptr<SmtpConnection> connection_;
    
    std::string server_;
    int port_;
    bool useSsl_;
    bool connected_;
    bool authenticated_;
    std::string username_;
    
    // Helper methods
    std::string sendCommand(const std::string& command);
    bool readResponse(int expectedCode = 250);
    std::string formatEmail(const std::string& from,
                           const std::string& to,
                           const std::string& subject,
                           const std::string& body);
};

} // namespace Pens

#endif // SMTP_CLIENT_HPP

