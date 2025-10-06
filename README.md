# Kvalog: Unified C++ Logger

A modern, header-only C++ logging library built on top of spdlog with flexible configuration and multiple output formats.

# kvalog

<p align="center">
  <img src="https://github.com/fogesque/kvalog/blob/main/logo.png?raw=true" alt="C++ kvalog logo" width="320"/>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17%2F20%2F23-blue.svg?logo=c%2B%2B&logoColor=white" alt="C++ Standard"/>
  <img src="https://img.shields.io/badge/License-MIT-green.svg" alt="MIT License"/>
</p>

A modern, header-only C++ logging library built on top of spdlog with flexible configuration and multiple output formats and targets.

## Features

- **Header-only**: Single header file for easy integration
- **Multiple output formats**: JSON and terminal-friendly formats
- **Flexible field configuration**: Enable/disable any log field at runtime
- **Multiple sinks**: Console, file, and network logging
- **Network adapters**: Interface-based design for HTTP, gRPC, or custom protocols
- **Synchronous and asynchronous modes**: Choose based on performance needs
- **Thread-safe**: Safe to use from multiple threads
- **Modern C++20**: Uses `std::source_location` for automatic file/line capture
- **No macros needed**: Clean API without preprocessor magic

## Requirements

- C++20 or later
- [spdlog](https://github.com/gabime/spdlog) library
- [nlohmann/json](https://github.com/nlohmann/json) library

## Installation

### Using CMake

```cmake
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(your_app main.cpp)
target_link_libraries(your_app PRIVATE 
    spdlog::spdlog 
    nlohmann_json::nlohmann_json
)
target_include_directories(your_app PRIVATE /path/to/kvalog)
```

### Using vcpkg

```bash
vcpkg install spdlog nlohmann-json
```

## Quick Start

```cpp
#include "kvalog.hpp"

using namespace kvalog;

int main() {
    // Create logger configuration
    Logger::Config config;
    config.format = OutputFormat::Terminal;
    config.logToConsole = true;

    // Set up context
    Logger::Context context;
    context.appName = "MyApp";
    context.moduleName = "NetworkModule";

    // Create logger instance
    Logger logger(config, context);
    
    // Log messages
    logger.info("Connection established");
    logger.error("Query failed");
    
    return 0;
}
```

## Configuration

### Log Fields

All fields are enabled by default and can be toggled at runtime:

```cpp
LogFieldConfig fields;
fields.includeAppName = true;      // Application name
fields.includeProcessId = true;    // Process ID
fields.includeThreadId = true;     // Thread ID
fields.includeModuleName = true;   // Module name (some part of application)
fields.includeLogLevel = true;     // Log level
fields.includeFile = true;         // Source file name and line
fields.includeMessage = true;      // Log message

config.fields = fields;
```

### Output Formats

#### Terminal Format
```
[MyApp][NetworkModule][PID:12345][TID:67890][INF][main.cpp:42] Connection established
```

#### JSON Format
```json
{
  "app": "MyApp",
  "process_id": 12345,
  "thread_id": "67890",
  "module": "NetworkModule",
  "level": "INF",
  "file": "main.cpp:42",
  "message": "Connection established"
}
```

### Output Destinations

#### Console Logging
```cpp
config.logToConsole = true;
```

#### File Logging
```cpp
config.logFilePath = "/var/log/myapp.log";
```

#### Network Logging

Implement the `NetworkSinkInterface` interface:

```cpp
class HttpAdapter : public NetworkSinkInterface {
public:
    void sendLog(const std::string& jsonLog) override {
        // Send JSON log via HTTP POST
    }
    
    bool isConnected() const override {
        return this->connected;
    }
private:
    bool connected;
};

auto adapter = std::make_shared<HttpAdapter>();
config.networkAdapter = adapter;
```

### Synchronous vs Asynchronous

#### Synchronous (default)
```cpp
config.asyncMode = Logger::Mode::Sync;
```

#### Asynchronous (better performance)
```cpp
config.asyncMode = Logger::Mode::Async;
config.asyncQueueSize = 8192;        // Queue size
config.asyncThreadCount = 4;         // Background threads
```

## Usage Examples

### Basic Logging

```cpp
Logger::Config config;
config.format = OutputFormat::Terminal;
config.logToConsole = true;

Logger::Context context;
context.appName = "WebServer";
context.moduleName = "Responder";

Logger logger(config, context);

logger.trace("Routing request");
logger.debug("Validating token");
logger.info("Processing request");
logger.warning("Cache miss");
logger.error("Connection timeout");
logger.critical("Out of memory");
```

### Runtime Field Configuration

```cpp
Logger logger(config);
logger.info("With all fields");

// Disable some fields
LogFieldConfig minimal;
minimal.includeProcessId = false;
minimal.includeThreadId = false;
minimal.includeFile = false;

logger.setFieldConfig(minimal);
logger.info("With minimal fields");
```

### Multiple Logger Instances

```cpp
// Main application logger
Logger::Config mainConfig;
Logger::Context mainContext;
mainContext.appName = "MainService";
Logger mainLogger(mainConfig, mainContext);

// Create subsystem logger with inherited config
Logger::Context dbContext;
dbContext.appName = "DatabaseService";
Logger dbLogger = Logger::WithConfigFrom(mainLogger, dbContext);
Logger::Context apiContext;
apiContext.appName = "ApiService";
Logger apiLogger = Logger::WithConfigFrom(mainLogger, apiContext);

mainLogger.info("Application started");
dbLogger.info("Database connected");
apiLogger.info("API server listening");
```

### Multi-threaded Async Logging

```cpp
Logger::Config config;
config.asyncMode = Logger::Mode::Async;
config.asyncQueueSize = 16384;
config.asyncThreadCount = 2;

Logger::Context ctx;
ctx.appName = "WorkerService";

Logger logger(config, ctx);

// Launch multiple worker threads
std::vector<std::thread> workers;
for (int i = 0; i < 10; ++i) {
    workers.emplace_back([&logger, i]() {
        for (int j = 0; j < 1000; ++j) {
            logger.info("Worker" + std::to_string(i) + 
                       "is processing item" + std::to_string(j));
        }
    });
}

for (auto & t : workers) {
    t.join();
}

logger.flush(); // Ensure all logs are written
```

### Network Logging with Multiple Adapters

```cpp
class GrpcLogAdapter : public NetworkSinkInterface {
public:
    void sendLog(const std::string & jsonLog) override {
        // Send JSON log via gRPC
    }
    bool isConnected() const override {
        return true;
    }
};

config.networkAdapter = std::make_shared<GrpcLogAdapter>();
Logger logger(config);
```

### Setting Log Levels

```cpp
Logger logger(config);

// Set minimum log level
logger.setLevel(LogLevel::Warning);

// These won't be logged
logger.trace("Not shown");
logger.debug("Not shown");
logger.info("Not shown");

// These will be logged
logger.warning("Shown");
logger.error("Shown");
logger.critical("Shown");
```

## Performance Considerations

### Async Mode
- Use async mode for high-throughput applications
- Reduces logging overhead by offloading to background threads
- Configure queue size based on expected log volume

### Field Selection
- Disable unused fields to reduce overhead
- JSON formatting is slightly more expensive than terminal format
- File and line info has minimal overhead with `std::source_location`

### Network Logging
- Implement connection pooling in your adapter
- Consider batching logs before sending
- Handle network failures gracefully

## Thread Safety

The logger is fully thread-safe. Multiple threads can log simultaneously without additional synchronization:

```cpp
Logger logger(config);

// Safe from multiple threads
std::thread t1([&]() { logger.info("Message 1"); });
std::thread t2([&]() { logger.info("Message 2"); });

t1.join();
t2.join();
```

## Best Practices

1. **Module naming**: Use consistent module names across your codebase
2. **Config reuse**: Create logger instances with `WithConfigFrom()` for consistency
3. **Async for performance**: Use async mode in production for better performance
4. **Flush on shutdown**: Always call `flush()` before application exit
5. **Network error handling**: Implement robust error handling in network adapters
6. **Log levels**: Use appropriate log levels (don't log everything as INFO)

## API Reference

### Logger::Config

```cpp
struct Config {
    OutputFormat format;                       // Json or Terminal
    LogFieldConfig fields;                     // Field configuration
    Mode asyncMode;                            // Sync or async logging
    bool logToConsole;                         // Enable console output
    std::optional<std::string> logFilePath;    // File path (optional)
    std::shared_ptr<NetworkSinkInterface> networkAdapter; // Network adapter (optional)
    size_t asyncQueueSize;                     // Async queue size
    size_t asyncThreadCount;                   // Async thread count
};
```

### Logger Methods

```cpp
void trace(const std::string& module, const std::string& message);
void debug(const std::string& module, const std::string& message);
void info(const std::string& module, const std::string& message);
void warning(const std::string& module, const std::string& message);
void error(const std::string& module, const std::string& message);
void critical(const std::string& module, const std::string& message);

void flush();

void setLevel(LogLevel level);
void setFieldConfig(const LogFieldConfig& fields);
void setOutputFormat(OutputFormat format);
```

## Summary

This is a customized way to use logging in our applications. We suggest unified interface to different logging targets. Main work of implementation was done by contrubutors of libraries [spdlog](https://github.com/gabime/spdlog) and [nlohmann/json](https://github.com/nlohmann/json).