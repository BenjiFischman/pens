#include "notification_processor.hpp"
#include "logger.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <map>
#include <thread>
#include <chrono>

namespace Pens {

NotificationProcessor::NotificationProcessor() 
    : priorityThreshold_(5), spamThreshold_(70) {
    LOG_INFO("Notification Processor initialized");
}

int NotificationProcessor::analyzeEmailPriority(const Email& email) {
    int priority = 5; // Default medium priority
    
    // Check for urgent keywords
    std::string upperSubject = email.subject;
    std::transform(upperSubject.begin(), upperSubject.end(), 
                   upperSubject.begin(), ::toupper);
    
    if (upperSubject.find("URGENT") != std::string::npos ||
        upperSubject.find("IMPORTANT") != std::string::npos ||
        upperSubject.find("CRITICAL") != std::string::npos) {
        priority += 3;
    }
    
    // Check for action items
    if (upperSubject.find("ACTION REQUIRED") != std::string::npos ||
        upperSubject.find("DEADLINE") != std::string::npos) {
        priority += 2;
    }
    
    // Reduce priority if likely spam
    if (isLikelySpam(email)) {
        priority = std::max(1, priority - 5);
    }
    
    return std::min(10, std::max(1, priority));
}

int NotificationProcessor::calculateSpamScore(const Email& email) {
    int spamScore = 0;
    
    // Check for common spam indicators
    std::vector<std::string> spamWords = {
        "FREE", "WIN", "WINNER", "CASH", "PRIZE", "CLICK HERE",
        "LIMITED TIME", "ACT NOW", "CONGRATULATIONS", "$$$",
        "VIAGRA", "CASINO", "LOTTERY"
    };
    
    std::string upperSubject = email.subject;
    std::transform(upperSubject.begin(), upperSubject.end(), 
                   upperSubject.begin(), ::toupper);
    
    for (const auto& word : spamWords) {
        if (upperSubject.find(word) != std::string::npos) {
            spamScore += 15;
        }
    }
    
    // Multiple exclamation marks
    int exclamations = std::count(email.subject.begin(), email.subject.end(), '!');
    if (exclamations > 2) {
        spamScore += exclamations * 5;
    }
    
    // All caps subject
    bool allCaps = !email.subject.empty() && 
                   std::all_of(email.subject.begin(), email.subject.end(),
                              [](char c) { return !std::isalpha(c) || std::isupper(c); });
    if (allCaps) spamScore += 20;
    
    return std::min(spamScore, 100);
}

std::string NotificationProcessor::categorizeEmail(const Email& email) {
    int spamScore = calculateSpamScore(email);
    
    if (spamScore > spamThreshold_) {
        return "Spam";
    } else if (email.subject.find("meeting") != std::string::npos ||
               email.subject.find("Meeting") != std::string::npos ||
               email.subject.find("invite") != std::string::npos) {
        return "Meeting";
    } else if (email.subject.find("invoice") != std::string::npos ||
               email.subject.find("Invoice") != std::string::npos ||
               email.subject.find("payment") != std::string::npos) {
        return "Financial";
    } else if (email.subject.find("newsletter") != std::string::npos ||
               email.subject.find("Newsletter") != std::string::npos) {
        return "Newsletter";
    } else if (containsUrgentKeywords(email.subject)) {
        return "Urgent";
    } else {
        return "General";
    }
}

std::vector<std::string> NotificationProcessor::extractKeywords(const Email& email) {
    std::vector<std::string> keywords;
    std::istringstream iss(email.subject);
    std::string word;
    
    while (iss >> word) {
        // Remove punctuation
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        
        // Only add significant words (length > 4)
        if (word.length() > 4) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            keywords.push_back(word);
        }
    }
    
    return keywords;
}

std::string NotificationProcessor::generateNotification(const Email& email) {
    std::ostringstream notification;
    
    int priority = analyzeEmailPriority(email);
    int spamScore = calculateSpamScore(email);
    std::string category = categorizeEmail(email);
    
    notification << "NEW EMAIL NOTIFICATION\n";
    notification << "━━━━━━━━━━━━━━━━━━━━━━━━\n";
    notification << "From: " << email.from << "\n";
    notification << "Subject: " << email.subject << "\n";
    notification << "Category: " << category << "\n";
    notification << "Priority: " << priority << "/10\n";
    
    if (spamScore > 0) {
        notification << "Spam Score: " << spamScore << "/100";
        if (spamScore > spamThreshold_) {
            notification << " (Likely Spam)";
        }
        notification << "\n";
    }
    
    notification << "Date: " << email.date << "\n";
    
    if (priority > priorityThreshold_) {
        notification << "⚠️  High Priority - Requires Attention\n";
    }
    
    notification << "━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    return notification.str();
}

std::string NotificationProcessor::generateBatchSummary(const std::vector<Email>& emails) {
    std::ostringstream summary;
    
    summary << "EMAIL BATCH SUMMARY\n";
    summary << "═══════════════════════════════════\n\n";
    summary << "Total Emails: " << emails.size() << "\n";
    
    // Calculate statistics
    int highPriority = 0;
    int mediumPriority = 0;
    int lowPriority = 0;
    int spamCount = 0;
    int unreadCount = 0;
    
    for (const auto& email : emails) {
        int priority = analyzeEmailPriority(email);
        int spamScore = calculateSpamScore(email);
        
        if (priority >= 8) highPriority++;
        else if (priority >= 5) mediumPriority++;
        else lowPriority++;
        
        if (spamScore > spamThreshold_) spamCount++;
        if (!email.isRead) unreadCount++;
    }
    
    summary << "Unread: " << unreadCount << "\n";
    summary << "High Priority: " << highPriority << "\n";
    summary << "Medium Priority: " << mediumPriority << "\n";
    summary << "Low Priority: " << lowPriority << "\n";
    summary << "Spam: " << spamCount << "\n\n";
    
    // Category breakdown
    std::map<std::string, int> categories;
    for (const auto& email : emails) {
        categories[categorizeEmail(email)]++;
    }
    
    summary << "Categories:\n";
    for (const auto& [category, count] : categories) {
        summary << "  " << category << ": " << count << "\n";
    }
    
    summary << "\n═══════════════════════════════════\n";
    
    return summary.str();
}

std::string NotificationProcessor::formatEmailSummary(const Email& email) {
    std::ostringstream summary;
    
    summary << "From: " << email.from << "\n";
    summary << "Subject: " << email.subject << "\n";
    summary << "Category: " << categorizeEmail(email) << "\n";
    summary << "Priority: " << analyzeEmailPriority(email) << "/10\n";
    
    return summary.str();
}

void NotificationProcessor::setPriorityThreshold(int threshold) {
    priorityThreshold_ = std::max(1, std::min(10, threshold));
    LOG_INFO("Priority threshold set to: " + std::to_string(priorityThreshold_));
}

int NotificationProcessor::getPriorityThreshold() const {
    return priorityThreshold_;
}

void NotificationProcessor::setSpamThreshold(int threshold) {
    spamThreshold_ = std::max(0, std::min(100, threshold));
    LOG_INFO("Spam threshold set to: " + std::to_string(spamThreshold_));
}

int NotificationProcessor::getSpamThreshold() const {
    return spamThreshold_;
}

bool NotificationProcessor::containsUrgentKeywords(const std::string& text) {
    std::vector<std::string> urgentWords = {
        "urgent", "important", "critical", "asap", "immediate",
        "deadline", "time-sensitive", "action required"
    };
    
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    
    for (const auto& word : urgentWords) {
        if (lowerText.find(word) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool NotificationProcessor::isLikelySpam(const Email& email) {
    return calculateSpamScore(email) > spamThreshold_;
}

int NotificationProcessor::calculateImportanceScore(const Email& email) {
    int score = analyzeEmailPriority(email);
    int spamScore = calculateSpamScore(email);
    
    // Reduce importance if likely spam
    if (spamScore > spamThreshold_) {
        score = std::max(1, score - 5);
    }
    
    return score;
}

// PensManager Implementation
PensManager::PensManager(std::shared_ptr<ImapClient> client,
                         std::shared_ptr<NotificationProcessor> processor)
    : client_(client),
      processor_(processor),
      running_(false),
      checkInterval_(60),
      processedCount_(0),
      unreadCount_(0) {
    
    LOG_INFO("PENS Manager initialized");
}

void PensManager::start() {
    running_ = true;
    LOG_INFO("PENS Manager started - monitoring for new emails");
    
    while (running_) {
        processNewEmails();
        std::this_thread::sleep_for(std::chrono::seconds(checkInterval_));
    }
}

void PensManager::stop() {
    running_ = false;
    LOG_INFO("PENS Manager stopped");
}

void PensManager::processNewEmails() {
    if (!client_->isConnected()) {
        LOG_WARNING("Not connected to IMAP server");
        return;
    }
    
    LOG_INFO("Checking for new emails...");
    
    auto emails = client_->fetchRecentEmails(10);
    
    if (!emails.empty()) {
        processEmailBatch(emails);
    } else {
        LOG_DEBUG("No new emails");
    }
}

void PensManager::setCheckInterval(int seconds) {
    checkInterval_ = std::max(10, seconds);
    LOG_INFO("Check interval set to: " + std::to_string(checkInterval_) + " seconds");
}

void PensManager::enableRealTimeNotifications(bool enable) {
    if (enable) {
        LOG_INFO("Real-time notifications enabled");
    } else {
        LOG_INFO("Real-time notifications disabled");
    }
}

void PensManager::setNotificationCallback(std::function<void(const std::string&)> callback) {
    notificationCallback_ = callback;
    LOG_INFO("Notification callback registered");
}

int PensManager::getProcessedEmailCount() const {
    return processedCount_;
}

int PensManager::getUnreadEmailCount() const {
    return unreadCount_;
}

std::string PensManager::getSystemStatus() const {
    std::ostringstream status;
    status << "PENS SYSTEM STATUS\n";
    status << "═══════════════════════════════════\n";
    status << "Running: " << (running_ ? "Yes" : "No") << "\n";
    status << "Connection: " << client_->getConnectionStatus() << "\n";
    status << "Emails Processed: " << processedCount_ << "\n";
    status << "Unread Emails: " << unreadCount_ << "\n";
    status << "Check Interval: " << checkInterval_ << " seconds\n";
    status << "Priority Threshold: " << processor_->getPriorityThreshold() << "/10\n";
    status << "Spam Threshold: " << processor_->getSpamThreshold() << "/100\n";
    status << "═══════════════════════════════════\n";
    return status.str();
}

void PensManager::processEmailBatch(const std::vector<Email>& emails) {
    LOG_INFO("Processing batch of " + std::to_string(emails.size()) + " emails");
    
    unreadCount_ = 0;
    for (const auto& email : emails) {
        if (!email.isRead) {
            unreadCount_++;
        }
        
        std::string notification = processor_->generateNotification(email);
        
        if (notificationCallback_) {
            notificationCallback_(notification);
        } else {
            std::cout << notification << std::endl;
        }
        
        processedCount_++;
    }
    
    std::string batchSummary = processor_->generateBatchSummary(emails);
    if (notificationCallback_) {
        notificationCallback_(batchSummary);
    } else {
        std::cout << batchSummary << std::endl;
    }
    
    LOG_INFO("Batch processing complete");
}

} // namespace Pens
