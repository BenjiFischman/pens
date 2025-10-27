#include "logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

namespace Pens {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() 
    : currentLevel_(LogLevel::INFO),
      consoleOutput_(true) {
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!shouldLog(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = getLevelString(level);
    std::string logMessage = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    if (consoleOutput_) {
        std::cout << logMessage << std::endl;
    }
    
    if (logFile_.is_open()) {
        logFile_ << logMessage << std::endl;
        logFile_.flush();
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel_ = level;
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (logFile_.is_open()) {
        logFile_.close();
    }
    
    logFile_.open(filename, std::ios::app);
    if (!logFile_.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

void Logger::enableConsoleOutput(bool enable) {
    consoleOutput_ = enable;
}

std::string Logger::getLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm = *std::localtime(&time);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

bool Logger::shouldLog(LogLevel level) const {
    return static_cast<int>(level) >= static_cast<int>(currentLevel_);
}

} // namespace Pens

