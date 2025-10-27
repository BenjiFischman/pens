#include "imap_client.hpp"
#include "notification_processor.hpp"
#include "config.hpp"
#include "logger.hpp"
#include <iostream>
#include <memory>
#include <csignal>
#include <thread>

using namespace Pens;

// Global flag for graceful shutdown
volatile sig_atomic_t running = 1;

void signalHandler(int signal) {
    LOG_INFO("Received signal " + std::to_string(signal) + ", shutting down...");
    running = 0;
}

void printBanner() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║   ██████╗ ███████╗███╗   ██╗███████╗                         ║
║   ██╔══██╗██╔════╝████╗  ██║██╔════╝                         ║
║   ██████╔╝█████╗  ██╔██╗ ██║███████╗                         ║
║   ██╔═══╝ ██╔══╝  ██║╚██╗██║╚════██║                         ║
║   ██║     ███████╗██║ ╚████║███████║                         ║
║   ╚═╝     ╚══════╝╚═╝  ╚═══╝╚══════╝                         ║
║                                                               ║
║        Professional Email Notification System                 ║
║                                                               ║
║        Intelligent email monitoring and notifications         ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
)" << std::endl;
}

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -c, --config FILE       Load configuration from FILE\n";
    std::cout << "  -s, --server SERVER     IMAP server address\n";
    std::cout << "  -p, --port PORT         IMAP port (default: 993)\n";
    std::cout << "  -u, --username USER     IMAP username\n";
    std::cout << "  -w, --password PASS     IMAP password\n";
    std::cout << "  -t, --threshold LEVEL   Set priority threshold (1-10, default: 5)\n";
    std::cout << "  -i, --interval SECONDS  Check interval (default: 60)\n";
    std::cout << "  -d, --debug             Enable debug mode\n";
    std::cout << "  -o, --once              Process once and exit\n";
    std::cout << "\nEnvironment Variables:\n";
    std::cout << "  PENS_IMAP_SERVER        IMAP server address\n";
    std::cout << "  PENS_IMAP_PORT          IMAP port\n";
    std::cout << "  PENS_IMAP_USERNAME      IMAP username\n";
    std::cout << "  PENS_IMAP_PASSWORD      IMAP password\n";
    std::cout << "  PENS_PRIORITY_THRESHOLD Priority threshold (1-10)\n";
    std::cout << "  PENS_CHECK_INTERVAL     Check interval in seconds\n";
    std::cout << "  PENS_DEBUG_MODE         Enable debug mode (true/false)\n";
    std::cout << "\nExample:\n";
    std::cout << "  " << program << " -s imap.gmail.com -u user@gmail.com -w password123\n";
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    printBanner();
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Get configuration
    Config& config = Config::getInstance();
    config.loadFromEnv();
    
    bool runOnce = false;
    bool showHelp = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            showHelp = true;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                config.loadFromFile(argv[++i]);
            }
        } else if (arg == "-s" || arg == "--server") {
            if (i + 1 < argc) {
                config.setImapServer(argv[++i]);
            }
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                config.setImapPort(std::stoi(argv[++i]));
            }
        } else if (arg == "-u" || arg == "--username") {
            if (i + 1 < argc) {
                std::string username = argv[++i];
                config.setImapCredentials(username, config.getImapPassword());
            }
        } else if (arg == "-w" || arg == "--password") {
            if (i + 1 < argc) {
                std::string password = argv[++i];
                config.setImapCredentials(config.getImapUsername(), password);
            }
        } else if (arg == "-t" || arg == "--threshold") {
            if (i + 1 < argc) {
                config.setPriorityThreshold(std::stoi(argv[++i]));
            }
        } else if (arg == "-d" || arg == "--debug") {
            Logger::getInstance().setLogLevel(LogLevel::DEBUG);
        } else if (arg == "-o" || arg == "--once") {
            runOnce = true;
        }
    }
    
    if (showHelp) {
        printUsage(argv[0]);
        return 0;
    }
    
    // Setup logger
    Logger& logger = Logger::getInstance();
    logger.setLogFile("pens.log");
    logger.enableConsoleOutput(true);
    
    if (config.getDebugMode()) {
        logger.setLogLevel(LogLevel::DEBUG);
    }
    
    LOG_INFO("Starting Professional Email Notification System (PENS)");
    
    // Validate configuration
    if (config.getImapUsername().empty() || config.getImapPassword().empty()) {
        LOG_ERROR("IMAP username and password are required!");
        LOG_INFO("Use command line arguments or environment variables to configure.");
        printUsage(argv[0]);
        return 1;
    }
    
    try {
        // Create IMAP client
        auto client = std::make_shared<ImapClient>(
            config.getImapServer(),
            config.getImapPort(),
            config.getImapUseSsl()
        );
        
        // Connect and authenticate
        LOG_INFO("Connecting to IMAP server...");
        if (!client->connect()) {
            LOG_ERROR("Failed to connect to IMAP server");
            return 1;
        }
        
        LOG_INFO("Authenticating...");
        if (!client->authenticate(config.getImapUsername(), config.getImapPassword())) {
            LOG_ERROR("Authentication failed");
            return 1;
        }
        
        LOG_INFO("Connected and authenticated successfully");
        
        // Create notification processor
        auto processor = std::make_shared<NotificationProcessor>();
        processor->setPriorityThreshold(config.getPriorityThreshold());
        
        // Create PENS manager
        auto manager = std::make_shared<PensManager>(client, processor);
        manager->setCheckInterval(config.getCheckInterval());
        
        // Print system status
        std::cout << manager->getSystemStatus() << std::endl;
        
        if (runOnce) {
            LOG_INFO("Processing emails once and exiting...");
            manager->processNewEmails();
        } else {
            LOG_INFO("Starting continuous email monitoring...");
            LOG_INFO("Press Ctrl+C to stop");
            
            // Run in a loop until interrupted
            while (running) {
                manager->processNewEmails();
                
                // Sleep for check interval
                for (int i = 0; i < config.getCheckInterval() && running; i++) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
            
            manager->stop();
        }
        
        LOG_INFO("Disconnecting...");
        client->disconnect();
        
    } catch (const std::exception& e) {
        LOG_CRITICAL("Fatal error: " + std::string(e.what()));
        return 1;
    }
    
    LOG_INFO("PENS shutdown complete");
    
    return 0;
}
