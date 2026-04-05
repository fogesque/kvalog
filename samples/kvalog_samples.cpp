#include <chrono>
#include <iostream>
#include <thread>

#include "kvalog.hpp"

using namespace kvalog;

// Example HTTP network adapter
class HttpNetworkAdapter : public INetworkSink
{
public:
    explicit HttpNetworkAdapter(const std::string & endpoint) : endpoint(endpoint), connected(true)
    {
    }

    void SendLog(const std::string & jsonLog) override
    {
        std::cout << "[HTTP -> " << this->endpoint << "] " << jsonLog << std::endl;
    }

    bool IsConnected() const override
    {
        return this->connected;
    }

private:
    std::string endpoint;
    bool connected = false;
};

// Example gRPC network adapter
class GrpcNetworkAdapter : public INetworkSink
{
public:
    explicit GrpcNetworkAdapter(const std::string & serverAddress)
        : serverAddress(serverAddress), connected(true)
    {
    }

    void SendLog(const std::string & jsonLog) override
    {
        std::cout << "[gRPC -> " << this->serverAddress << "] " << jsonLog << std::endl;
    }

    bool IsConnected() const override
    {
        return this->connected;
    }

private:
    std::string serverAddress;
    bool connected = false;
};

void sampleBasicUsage()
{
    std::cout << "\n=== Basic Usage Sample ===" << std::endl;

    auto config = Logger::Config();
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;

    auto context = Logger::Context();
    context.appName = "MyApplication";
    context.moduleName = "MainModule";

    auto logger = Logger::Create(config, context);

    logger->Info("Application started successfully");
    logger->Debug("Connected to database");
    logger->Warning("Cache miss for key: user_123");
    logger->Error("Failed to connect to remote service");
}

void sampleJsonLogging()
{
    std::cout << "\n=== JSON Format Sample ===" << std::endl;

    auto config = Logger::Config();
    config.format = OutputFormat::Json;
    config.logToConsole = true;

    auto context = Logger::Context();
    context.appName = "JsonApp";
    context.moduleName = "MainModule";

    auto logger = Logger::Create(config, context);

    logger->Info("User logged in");
    logger->Error("Transaction failed");
}

void sampleFieldConfiguration()
{
    std::cout << "\n=== Field Configuration Sample ===" << std::endl;

    auto config = Logger::Config();
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;

    config.fields.includeProcessId = false;
    config.fields.includeThreadId = false;
    config.fields.includeFile = false;

    auto context = Logger::Context();
    context.appName = "MinimalApp";
    context.moduleName = "MainModule";

    auto logger = Logger::Create(config, context);

    logger->Info("Button clicked");

    auto newFields = LogFieldConfig();
    newFields.includeProcessId = true;
    newFields.includeThreadId = true;
    logger->SetFieldConfig(newFields);

    logger->Info("Window resized");
}

void sampleFileLogging()
{
    std::cout << "\n=== File Logging Sample ===" << std::endl;

    auto config = Logger::Config();
    config.format = OutputFormat::Json;
    config.logToConsole = true;
    config.logFilePath = "application.log";

    auto context = Logger::Context();
    context.appName = "FileApp";
    context.moduleName = "MainModule";

    auto logger = Logger::Create(config, context);

    logger->Info("Logging to both console and file");
    logger->Debug("Configuration loaded");

    std::cout << "Check 'application.log' file for JSON output" << std::endl;
}

void sampleNetworkLogging()
{
    std::cout << "\n=== Network Logging Sample ===" << std::endl;

    auto httpAdapter = std::make_shared<HttpNetworkAdapter>("http://logstash.example.com:5000");

    auto config = Logger::Config();
    config.format = OutputFormat::Json;
    config.logToConsole = false;
    config.networkAdapter = httpAdapter;

    auto context = Logger::Context();
    context.appName = "NetworkApp";
    context.moduleName = "MainModule";

    auto logger = Logger::Create(config, context);

    logger->Info("Request received");
    logger->Error("Invalid input data");

    std::cout << "\n--- Switching to gRPC ---" << std::endl;
    auto grpcAdapter = std::make_shared<GrpcNetworkAdapter>("localhost:50051");

    auto grpcConfig = Logger::Config();
    grpcConfig.format = OutputFormat::Json;
    grpcConfig.networkAdapter = grpcAdapter;

    auto grpcContext = Logger::Context();
    grpcContext.appName = "GrpcApp";
    grpcContext.moduleName = "MainModule";

    auto grpcLogger = Logger::Create(grpcConfig, grpcContext);
    grpcLogger->Info("Streaming data");
}

void sampleAsyncLogging()
{
    std::cout << "\n=== Async Logging Sample ===" << std::endl;

    auto config = Logger::Config();
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;
    config.asyncMode = Logger::Mode::Async;
    config.asyncQueueSize = DefaultAsyncQueueSize;
    config.asyncThreadCount = 2;

    auto context = Logger::Context();
    context.appName = "AsyncApp";
    context.moduleName = "MainModule";

    auto logger = Logger::Create(config, context);

    auto worker = [&logger](int workerId) {
        for (int i = 0; i < 5; ++i) {
            logger->Info("Worker" + std::to_string(workerId) + " is processing item" +
                         std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };

    auto thread1 = std::thread(worker, 1);
    auto thread2 = std::thread(worker, 2);
    auto thread3 = std::thread(worker, 3);

    thread1.join();
    thread2.join();
    thread3.join();

    logger->Flush();
}

void sampleCopyConfig()
{
    std::cout << "\n=== Copy Config Sample ===" << std::endl;

    auto config = Logger::Config();
    config.format = OutputFormat::Terminal;
    config.fields.includeProcessId = false;
    config.logToConsole = true;

    auto context = Logger::Context();
    context.appName = "MainService";
    context.moduleName = "MainModule";

    auto mainLogger = Logger::Create(config, context);
    mainLogger->Info("Main service started");

    auto newContext = Logger::Context();
    newContext.appName = "SubService";
    newContext.moduleName = "MainModule";

    auto subLogger = Logger::WithConfigFrom(*mainLogger, newContext);
    subLogger.Info("Sub service started with inherited config");
}

void sampleMultipleSinks()
{
    std::cout << "\n=== Multiple Sinks Sample ===" << std::endl;

    auto networkAdapter = std::make_shared<HttpNetworkAdapter>("http://logs.example.com");

    auto config = Logger::Config();
    config.format = OutputFormat::Json;
    config.logToConsole = true;
    config.logFilePath = "multi_sink.log";
    config.networkAdapter = networkAdapter;

    auto context = Logger::Context();
    context.appName = "MultiSinkApp";
    context.moduleName = "MainModule";

    auto logger = Logger::Create(config, context);

    logger->Info("This goes to console, file, and network!");
    logger->Critical("Critical Error logged everywhere");
}

void sampleLogLevels()
{
    std::cout << "\n=== Log Levels Sample ===" << std::endl;

    auto config = Logger::Config();
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;

    auto logger = Logger::Create(config);

    logger->Trace("Trace level message");
    logger->Debug("Debug level message");
    logger->Info("Info level message");
    logger->Warning("Warning level message");
    logger->Error("Error level message");
    logger->Critical("Critical level message");

    std::cout << "\n--- Setting minimum level to Warning ---" << std::endl;
    logger->SetLevel(LogLevel::Warning);

    logger->Debug("This won't be shown");
    logger->Info("This won't be shown either");
    logger->Warning("This will be shown");
    logger->Error("This will be shown too");

    std::cout << "\n--- Setting minimum level to Off ---" << std::endl;
    logger->SetLevel(LogLevel::Off);

    logger->Warning("This won't be shown");
    logger->Error("This won't be shown too");
}

void sampleColoredOutput()
{
    std::cout << "\n=== Colored Output Sample ===" << std::endl;

    auto config = Logger::Config();
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;
    config.enableColors = true;

    auto context = Logger::Context();
    context.appName = "ColorApp";
    context.moduleName = "MainModule";

    auto logger = Logger::Create(config, context);

    logger->Trace("Trace level message");
    logger->Debug("Debug level message");
    logger->Info("Info level message");
    logger->Warning("Warning level message");
    logger->Error("Error level message");
    logger->Critical("Critical level message");
}

void sampleFormattedLogging()
{
    std::cout << "\n=== Formatted Logging Sample ===" << std::endl;

    auto config = Logger::Config();
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;
    config.enableColors = true;

    auto context = Logger::Context();
    context.appName = "FormatApp";
    context.moduleName = "MainModule";

    auto logger = Logger::Create(config, context);

    // Plain string (no formatting)
    logger->Info("Application started successfully");

    // Formatted with arguments
    const auto username = std::string("alice");
    const auto ip = std::string("192.168.1.42");
    logger->Info("User '{}' logged in from {}", username, ip);

    logger->Warning("Cache miss rate: {:.1f}%", 12.5);
    logger->Error("Connection to {} failed after {} retries", "db-primary", 3);
    logger->Debug("Processing batch {}/{}", 7, 10);
}

void sampleFabricMethods()
{
    std::cout << "\n=== Fabric Methods Sample ===" << std::endl;

    auto logger = kvalog::CreateLogger("MyApp", "Main");

    logger->Info("Application started successfully");
    logger->Debug("Connected to database");
    logger->Warning("Cache miss for key: {}", "user_123");
    logger->Error("Failed to connect to remote service");
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
    sampleColoredOutput();
    sampleFormattedLogging();
    sampleFabricMethods();

    std::cout << "\n=== All Samples Completed ===" << std::endl;

    return 0;
}
