#ifndef IMAP_CLIENT_HPP
#define IMAP_CLIENT_HPP

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace Pens {

/**
 * @brief Email structure representing a fetched email
 */
struct Email {
    std::string id;
    std::string from;
    std::string subject;
    std::string body;
    std::string date;
    bool isRead;
    int priority;  // Email priority score
};

/**
 * @brief IMAP Client for connecting to email servers
 * 
 * This is the Silly Email Notification System (SENS) IMAP client
 * that connects to servers and processes emails in amusing ways.
 */
class ImapClient {
public:
    ImapClient(const std::string& server, int port, bool useSsl = true);
    ~ImapClient();

    // Connection management
    bool connect();
    bool authenticate(const std::string& username, const std::string& password);
    bool disconnect();
    bool isConnected() const;

    // Mailbox operations
    bool selectMailbox(const std::string& mailbox = "INBOX");
    std::vector<std::string> listMailboxes();
    int getMessageCount();

    // Email operations
    std::vector<Email> fetchRecentEmails(int count = 10);
    Email fetchEmail(const std::string& uid);
    bool markAsRead(const std::string& uid);
    bool deleteEmail(const std::string& uid);

    // Status and monitoring
    std::string getConnectionStatus() const;
    
private:
    struct ImapConnection;
    std::unique_ptr<ImapConnection> connection_;
    
    std::string server_;
    int port_;
    bool useSsl_;
    bool connected_;
    bool authenticated_;
    std::string currentMailbox_;

    // Helper methods
    std::string sendCommand(const std::string& command);
    bool parseResponse(const std::string& response);
    Email parseEmailData(const std::string& data, const std::string& uid);
    int calculatePriorityScore(const Email& email);
};

} // namespace Pens

#endif // IMAP_CLIENT_HPP

