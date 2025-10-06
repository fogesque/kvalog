#include <chrono>
#include <iostream>
#include <thread>

#include "kvalog.hpp"

using namespace kvalog;

// Example HTTP network adapter
class HttpNetworkAdapter : public NetworkSinkInterface
{
public:
    explicit HttpNetworkAdapter(const std::string & endpoint) : endpoint(endpoint), connected(true)
    {
    }

    void sendLog(const std::string & jsonLog) override
    {
        // In real implementation: HTTP POST to endpoint
        std::cout << "[HTTP -> " << this->endpoint << "] " << jsonLog << std::endl;
    }

    bool isConnected() const override
    {
        return this->connected;
    }

private:
    std::string endpoint;
    bool connected;
};

// Example gRPC network adapter
class GrpcNetworkAdapter : public NetworkSinkInterface
{
public:
    explicit GrpcNetworkAdapter(const std::string & serverAddress)
        : serverAddress(serverAddress), connected(true)
    {
    }

    void sendLog(const std::string & jsonLog) override
    {
        // In real implementation: gRPC call
        std::cout << "[gRPC -> " << this->serverAddress << "] " << jsonLog << std::endl;
    }

    bool isConnected() const override
    {
        return this->connected;
    }

private:
    std::string serverAddress;
    bool connected;
};

void exampleBasicUsage()
{
    std::cout << "\n=== Basic Usage Example ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;

    Logger::Context ctx;
    ctx.appName = "MyApplication";
    ctx.moduleName = "MainModule";

    Logger logger(config, ctx);

    logger.info("Application started successfully");
    logger.debug("Connected to database");
    logger.warning("Cache miss for key: user_123");
    logger.error("Failed to connect to remote service");
}

void exampleJsonLogging()
{
    std::cout << "\n=== JSON Format Example ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Json;
    config.logToConsole = true;

    Logger::Context ctx;
    ctx.appName = "JsonApp";
    ctx.moduleName = "MainModule";

    Logger logger(config, ctx);

    logger.info("User logged in");
    logger.error("Transaction failed");
}

void exampleFieldConfiguration()
{
    std::cout << "\n=== Field Configuration Example ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;

    // Disable some fields
    config.fields.includeProcessId = false;
    config.fields.includeThreadId = false;
    config.fields.includeFile = false;

    Logger::Context ctx;
    ctx.appName = "MinimalApp";
    ctx.moduleName = "MainModule";

    Logger logger(config, ctx);

    logger.info("Button clicked");

    // Re-enable fields at runtime
    LogFieldConfig newFields;
    newFields.includeProcessId = true;
    newFields.includeThreadId = true;
    logger.SetFieldConfig(newFields);

    logger.info("Window resized");
}

void exampleFileLogging()
{
    std::cout << "\n=== File Logging Example ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Json;
    config.logToConsole = true;
    config.logFilePath = "application.log";

    Logger::Context ctx;
    ctx.appName = "FileApp";
    ctx.moduleName = "MainModule";

    Logger logger(config, ctx);

    logger.info("Logging to both console and file");
    logger.debug("Configuration loaded");

    std::cout << "Check 'application.log' file for JSON output" << std::endl;
}

void exampleNetworkLogging()
{
    std::cout << "\n=== Network Logging Example ===" << std::endl;

    // HTTP adapter
    auto httpAdapter = std::make_shared<HttpNetworkAdapter>("http://logstash.example.com:5000");

    Logger::Config config;
    config.format = OutputFormat::Json;
    config.logToConsole = false;  // Only network
    config.networkAdapter = httpAdapter;

    Logger::Context ctx;
    ctx.appName = "NetworkApp";
    ctx.moduleName = "MainModule";

    Logger logger(config, ctx);

    logger.info("Request received");
    logger.error("Invalid input data");

    // Switch to gRPC adapter
    std::cout << "\n--- Switching to gRPC ---" << std::endl;
    auto grpcAdapter = std::make_shared<GrpcNetworkAdapter>("localhost:50051");

    Logger::Config grpcConfig;
    grpcConfig.format = OutputFormat::Json;
    grpcConfig.networkAdapter = grpcAdapter;

    Logger::Context grpcCtx;
    grpcCtx.appName = "GrpcApp";
    grpcCtx.moduleName = "MainModule";

    Logger grpcLogger(grpcConfig, grpcCtx);
    grpcLogger.info("Streaming data");
}

void exampleAsyncLogging()
{
    std::cout << "\n=== Async Logging Example ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;
    config.asyncMode = Logger::Mode::Async;
    config.asyncQueueSize = 8192;
    config.asyncThreadCount = 2;

    Logger::Context ctx;
    ctx.appName = "AsyncApp";
    ctx.moduleName = "MainModule";

    Logger logger(config, ctx);

    // Log from multiple threads
    auto worker = [&logger](int workerId) {
        for (int i = 0; i < 5; ++i) {
            logger.info("Worker" + std::to_string(workerId) + " is processing item" +
                        std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);

    t1.join();
    t2.join();
    t3.join();

    logger.flush();  // Ensure all async logs are written
}

void exampleCopyConfig()
{
    std::cout << "\n=== Copy Config Example ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Terminal;
    config.fields.includeProcessId = false;
    config.logToConsole = true;

    Logger::Context ctx;
    ctx.appName = "MainService";
    ctx.moduleName = "MainModule";

    Logger mainLogger(config, ctx);
    mainLogger.info("Main service started");

    Logger::Context newCtx;
    newCtx.appName = "SubService";
    newCtx.moduleName = "MainModule";

    // Create new logger with same config but different app name
    Logger subLogger = Logger::WithConfigFrom(mainLogger, newCtx);
    subLogger.info("Sub service started with inherited config");
}

void exampleMultipleSinks()
{
    std::cout << "\n=== Multiple Sinks Example ===" << std::endl;

    auto networkAdapter = std::make_shared<HttpNetworkAdapter>("http://logs.example.com");

    Logger::Config config;
    config.format = OutputFormat::Json;
    config.logToConsole = true;
    config.logFilePath = "multi_sink.log";
    config.networkAdapter = networkAdapter;

    Logger::Context ctx;
    ctx.appName = "MultiSinkApp";
    ctx.moduleName = "MainModule";

    Logger logger(config, ctx);

    logger.info("This goes to console, file, and network!");
    logger.critical("Critical error logged everywhere");
}

void exampleLogLevels()
{
    std::cout << "\n=== Log Levels Example ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;

    Logger::Context ctx;
    ctx.appName = "LevelApp";
    ctx.moduleName = "MainModule";

    Logger logger(config);

    // Log at all levels
    logger.trace("Trace level message");
    logger.debug("Debug level message");
    logger.info("Info level message");
    logger.warning("Warning level message");
    logger.error("Error level message");
    logger.critical("Critical level message");

    std::cout << "\n--- Setting minimum level to Warning ---" << std::endl;
    logger.setLevel(LogLevel::Warning);

    logger.debug("This won't be shown");
    logger.info("This won't be shown either");
    logger.warning("This will be shown");
    logger.error("This will be shown too");
}

int main()
{
    std::cout << "=== Unified Logger Examples ===" << std::endl;

    exampleBasicUsage();
    exampleJsonLogging();
    exampleFieldConfiguration();
    exampleFileLogging();
    exampleNetworkLogging();
    exampleAsyncLogging();
    exampleCopyConfig();
    exampleMultipleSinks();
    exampleLogLevels();

    std::cout << "\n=== All Examples Completed ===" << std::endl;

    return 0;
}