#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

namespace Pens {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

/**
 * @brief Simple logger for SENS
 */
class Logger {
public:
    static Logger& getInstance();
    
    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filename);
    void enableConsoleOutput(bool enable);
    
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    LogLevel currentLevel_;
    std::ofstream logFile_;
    bool consoleOutput_;
    std::mutex mutex_;
    
    std::string getLevelString(LogLevel level) const;
    std::string getCurrentTimestamp() const;
    bool shouldLog(LogLevel level) const;
};

// Convenience macros
#define LOG_DEBUG(msg) Pens::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) Pens::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) Pens::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) Pens::Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) Pens::Logger::getInstance().critical(msg)

} // namespace Pens

#endif // LOGGER_HPP

