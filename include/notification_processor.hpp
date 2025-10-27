#ifndef NOTIFICATION_PROCESSOR_HPP
#define NOTIFICATION_PROCESSOR_HPP

#include "imap_client.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Pens {

/**
 * @brief Professional Email Notification Processor
 * 
 * Analyzes emails and generates professional notifications with
 * priority classification and spam detection.
 */
class NotificationProcessor {
public:
    NotificationProcessor();
    
    // Email analysis methods
    int analyzeEmailPriority(const Email& email);
    int calculateSpamScore(const Email& email);
    std::string categorizeEmail(const Email& email);
    std::vector<std::string> extractKeywords(const Email& email);
    
    // Notification generation
    std::string generateNotification(const Email& email);
    std::string generateBatchSummary(const std::vector<Email>& emails);
    std::string formatEmailSummary(const Email& email);
    
    // Configuration
    void setPriorityThreshold(int threshold);
    int getPriorityThreshold() const;
    void setSpamThreshold(int threshold);
    int getSpamThreshold() const;
    
private:
    int priorityThreshold_;
    int spamThreshold_;
    
    // Helper methods
    bool containsUrgentKeywords(const std::string& text);
    bool isLikelySpam(const Email& email);
    int calculateImportanceScore(const Email& email);
};

/**
 * @brief Professional Email Notification System (PENS) Manager
 * 
 * Coordinates the IMAP client and notification processor to provide
 * a complete email notification and monitoring system.
 */
class PensManager {
public:
    PensManager(std::shared_ptr<ImapClient> client, 
                std::shared_ptr<NotificationProcessor> processor);
    
    // Main operations
    void start();
    void stop();
    void processNewEmails();
    
    // Configuration
    void setCheckInterval(int seconds);
    void enableRealTimeNotifications(bool enable);
    void setNotificationCallback(std::function<void(const std::string&)> callback);
    
    // Statistics
    int getProcessedEmailCount() const;
    int getUnreadEmailCount() const;
    std::string getSystemStatus() const;
    
private:
    std::shared_ptr<ImapClient> client_;
    std::shared_ptr<NotificationProcessor> processor_;
    bool running_;
    int checkInterval_;
    int processedCount_;
    int unreadCount_;
    std::function<void(const std::string&)> notificationCallback_;
    
    void processEmailBatch(const std::vector<Email>& emails);
};

} // namespace Pens

#endif // NOTIFICATION_PROCESSOR_HPP
