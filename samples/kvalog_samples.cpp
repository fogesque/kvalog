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

    void SendLog(const std::string & jsonLog) override
    {
        // In real implementation: HTTP POST to endpoint
        std::cout << "[HTTP -> " << this->endpoint << "] " << jsonLog << std::endl;
    }

    bool IsConnected() const override
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

    void SendLog(const std::string & jsonLog) override
    {
        // In real implementation: gRPC call
        std::cout << "[gRPC -> " << this->serverAddress << "] " << jsonLog << std::endl;
    }

    bool IsConnected() const override
    {
        return this->connected;
    }

private:
    std::string serverAddress;
    bool connected;
};

void sampleBasicUsage()
{
    std::cout << "\n=== Basic Usage Sample ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;

    Logger::Context ctx;
    ctx.appName = "MyApplication";
    ctx.moduleName = "MainModule";

    Logger logger(config, ctx);

    logger.Info("Application started successfully");
    logger.Debug("Connected to database");
    logger.Warning("Cache miss for key: user_123");
    logger.Error("Failed to connect to remote service");
}

void sampleJsonLogging()
{
    std::cout << "\n=== JSON Format Sample ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Json;
    config.logToConsole = true;

    Logger::Context ctx;
    ctx.appName = "JsonApp";
    ctx.moduleName = "MainModule";

    Logger logger(config, ctx);

    logger.Info("User logged in");
    logger.Error("Transaction failed");
}

void sampleFieldConfiguration()
{
    std::cout << "\n=== Field Configuration Sample ===" << std::endl;

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

    logger.Info("Button clicked");

    // Re-enable fields at runtime
    LogFieldConfig newFields;
    newFields.includeProcessId = true;
    newFields.includeThreadId = true;
    logger.SetFieldConfig(newFields);

    logger.Info("Window resized");
}

void sampleFileLogging()
{
    std::cout << "\n=== File Logging Sample ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Json;
    config.logToConsole = true;
    config.logFilePath = "application.log";

    Logger::Context ctx;
    ctx.appName = "FileApp";
    ctx.moduleName = "MainModule";

    Logger logger(config, ctx);

    logger.Info("Logging to both console and file");
    logger.Debug("Configuration loaded");

    std::cout << "Check 'application.log' file for JSON output" << std::endl;
}

void sampleNetworkLogging()
{
    std::cout << "\n=== Network Logging Sample ===" << std::endl;

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

    logger.Info("Request received");
    logger.Error("Invalid input data");

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
    grpcLogger.Info("Streaming data");
}

void sampleAsyncLogging()
{
    std::cout << "\n=== Async Logging Sample ===" << std::endl;

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
            logger.Info("Worker" + std::to_string(workerId) + " is processing item" +
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

    logger.Flush();  // Ensure all async logs are written
}

void sampleCopyConfig()
{
    std::cout << "\n=== Copy Config Sample ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Terminal;
    config.fields.includeProcessId = false;
    config.logToConsole = true;

    Logger::Context ctx;
    ctx.appName = "MainService";
    ctx.moduleName = "MainModule";

    Logger mainLogger(config, ctx);
    mainLogger.Info("Main service started");

    Logger::Context newCtx;
    newCtx.appName = "SubService";
    newCtx.moduleName = "MainModule";

    // Create new logger with same config but different app name
    Logger subLogger = Logger::WithConfigFrom(mainLogger, newCtx);
    subLogger.Info("Sub service started with inherited config");
}

void sampleMultipleSinks()
{
    std::cout << "\n=== Multiple Sinks Sample ===" << std::endl;

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

    logger.Info("This goes to console, file, and network!");
    logger.Critical("Critical Error logged everywhere");
}

void sampleLogLevels()
{
    std::cout << "\n=== Log Levels Sample ===" << std::endl;

    Logger::Config config;
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;

    Logger::Context ctx;
    ctx.appName = "LevelApp";
    ctx.moduleName = "MainModule";

    Logger logger(config);

    // Log at all levels
    logger.Trace("Trace level message");
    logger.Debug("Debug level message");
    logger.Info("Info level message");
    logger.Warning("Warning level message");
    logger.Error("Error level message");
    logger.Critical("Critical level message");

    std::cout << "\n--- Setting minimum level to Warning ---" << std::endl;
    logger.SetLevel(LogLevel::Warning);

    logger.Debug("This won't be shown");
    logger.Info("This won't be shown either");
    logger.Warning("This will be shown");
    logger.Error("This will be shown too");

    std::cout << "\n--- Setting minimum level to Off ---" << std::endl;
    logger.SetLevel(LogLevel::Off);

    logger.Warning("This won't be shown");
    logger.Error("This won't be shown too");
}

void sampleFabricMethods() {
    std::cout << "\n=== Fabric Methods Sample ===" << std::endl;

    auto logger = kvalog::CreateLogger("MyApp", "Main");
        
    logger.Info("Application started successfully");
    logger.Debug("Connected to database");
    logger.Warning("Cache miss for key: user_123");
    logger.Error("Failed to connect to remote service");

}

int main()
{
    std::cout << "=== Unified Logger Samples ===" << std::endl;

    sampleBasicUsage();
    sampleJsonLogging();
    sampleFieldConfiguration();
    sampleFileLogging();
    sampleNetworkLogging();
    sampleAsyncLogging();
    sampleCopyConfig();
    sampleMultipleSinks();
    sampleLogLevels(); 
    sampleFabricMethods();

    std::cout << "\n=== All Samples Completed ===" << std::endl;

    return 0;
}