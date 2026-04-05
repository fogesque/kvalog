# kvalog

<p align="center">
  <img src="https://github.com/fogesque/kvalog/blob/main/logo.png?raw=true" alt="C++ kvalog logo" width="320"/>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17%2F20%2F23-blue.svg?logo=c%2B%2B&logoColor=white" alt="C++ Standard"/>
  <img src="https://img.shields.io/badge/License-MIT-green.svg" alt="MIT License"/>
</p>

Header-only C++ logging library built on top of spdlog with flexible configuration and multiple output formats and targets.

## Features

- **Header-only**: Single header file for easy integration
- **Logging profiles**: Predefined configurations for common use cases
- **Formatted messages**: `fmt`-style format strings with automatic source location capture
- **Multiple output formats**: JSON and terminal-friendly formats
- **Colored terminal output**: Per-level coloring with opt-in configuration
- **Flexible field configuration**: Enable/disable any log field at runtime
- **Multiple sinks**: Console, file, and network logging
- **Network adapters**: Interface-based design for HTTP, gRPC, or custom protocols
- **Synchronous and asynchronous modes**: Choose based on performance needs
- **Thread-safe**: Safe to use from multiple threads
- **No macros needed**: Clean API without preprocessor magic

## Requirements

- C++20 or later
- [spdlog](https://github.com/gabime/spdlog) library (if using without vcpkg)
- [nlohmann/json](https://github.com/nlohmann/json) library (if using without vcpkg)

## Installation

Kvalog is supported via vcpkg and can be added as overlay port from custom [fogesque vcpkg overlay ports](https://github.com/fogesque/vcpkg-ports).

### Using CMake With vcpkg Toolchain

```cmake
find_package(kvalog REQUIRED)

add_executable(your_app main.cpp)
target_link_libraries(your_app PRIVATE 
    kvalog::kvalog 
)
```

### Using vcpkg

```bash
vcpkg install kvalog
```

## Quick Start

```cpp
#include <kvalog/kvalog.hpp>

int main()
{
    // Create a logger with a profile
    const auto context = kvalog::Logger::Context{ .appName = "MyApp", .moduleName = "Main" };
    auto logger = kvalog::CreateLogger(kvalog::LogProfile::ColoredDefault, context);

    // Plain messages
    logger->Info("Application started");

    // Formatted messages with arguments
    logger->Info("User '{}' logged in from {}", "alice", "192.168.1.42");
    logger->Warning("Cache miss rate: {:.1f}%", 12.5);
    logger->Error("Connection to {} failed after {} retries", "db-primary", 3);

    return 0;
}
```

## Logging Profiles

Profiles are predefined configurations for common use cases. Use `CreateLogger` with a `LogProfile` to get started quickly:

```cpp
const auto context = kvalog::Logger::Context{ .appName = "MyApp", .moduleName = "Auth" };
auto logger = kvalog::CreateLogger(kvalog::LogProfile::ColoredDefault, context);
```

### Available Profiles

| Profile | Format | Colors | Time | PID/TID | File | Description |
|---|---|---|---|---|---|---|
| `Minimal` | Terminal | No | No | No | No | Least noise, just app + level + message |
| `Default` | Terminal | No | No | No | Yes | Balanced everyday output |
| `Detailed` | Terminal | No | Yes | No | Yes | Timestamps without process/thread IDs |
| `Verbose` | Terminal | No | Yes | Yes | Yes | Full diagnostics with all fields |
| `ColoredDefault` | Terminal | Yes | No | No | Yes | Default with colored level tags |
| `ColoredDetailed` | Terminal | Yes | Yes | No | Yes | Detailed with colored level tags |
| `ColoredVerbose` | Terminal | Yes | Yes | Yes | Yes | Verbose with colored level tags |
| `Json` | JSON | No | Yes | Yes | Yes | Structured JSON for log aggregation |

### Profile Output Examples

**Minimal**
```
[MyApp][INF] Application started
```

**Default**
```
[MyApp][Auth][INF][main.cpp:42] Application started
```

**Detailed**
```
[2025-10-06 21:58:46.529][MyApp][Auth][INF][main.cpp:42] Application started
```

**Verbose**
```
[2025-10-06 21:58:46.529][MyApp][Auth][PID:12345][TID:12345][INF][main.cpp:42] Application started
```

**ColoredDefault** / **ColoredDetailed** / **ColoredVerbose**

Same layout as their non-colored counterparts, but with bright white text and per-level colored tags:
- `TRC` light velvet
- `DBG` light cyan
- `INF` light blue
- `WRN` yellow
- `ERR` red
- `CRT` bold red

**Json**
```json
{"app":"MyApp","file":"main.cpp:42","level":"INF","message":"Application started","module":"Auth","process_id":"12345","thread_id":"12345","time":"2025-10-06 21:58:46.529"}
```

### Custom Configuration

If profiles don't fit your needs, create a `Logger::Config` directly:

```cpp
auto config = kvalog::Logger::Config();
config.format = kvalog::OutputFormat::Terminal;
config.logToConsole = true;
config.enableColors = true;
config.logFilePath = "/var/log/myapp.log";
config.fields.includeProcessId = false;
config.fields.includeThreadId = false;
config.fields.includeTime = true;

const auto context = kvalog::Logger::Context{ .appName = "MyApp", .moduleName = "Main" };
auto logger = kvalog::Logger::Create(config, context);
```

## Formatted Messages

Logging methods accept `fmt`-style format strings with variadic arguments. Source location is captured automatically at the call site.

```cpp
auto logger = kvalog::CreateLogger(kvalog::LogProfile::Default, context);

// Plain string
logger->Info("Server started");

// Format arguments
logger->Info("Listening on {}:{}", "0.0.0.0", 8080);
logger->Debug("Request handled in {:.2f}ms", 3.14);
logger->Warning("Disk usage at {}%", 95);
logger->Error("Failed to parse config: {}", errorMessage);
```

## Configuration

### Log Fields

All fields are enabled by default and can be toggled at runtime:

```cpp
kvalog::LogFieldConfig fields;
fields.includeAppName = true;
fields.includeProcessId = true;
fields.includeThreadId = true;
fields.includeModuleName = true;
fields.includeLogLevel = true;
fields.includeFile = true;
fields.includeMessage = true;
fields.includeTime = true;

config.fields = fields;
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

Implement the `INetworkSink` interface:

```cpp
class HttpAdapter : public kvalog::INetworkSink
{
public:
    void SendLog(const std::string & jsonLog) override
    {
        // Send JSON log via HTTP POST
    }

    bool IsConnected() const override
    {
        return this->connected;
    }

private:
    bool connected = false;
};

auto adapter = std::make_shared<HttpAdapter>();
config.networkAdapter = adapter;
```

### Synchronous vs Asynchronous

#### Synchronous (default)
```cpp
config.asyncMode = kvalog::Logger::Mode::Sync;
```

#### Asynchronous
```cpp
config.asyncMode = kvalog::Logger::Mode::Async;
config.asyncQueueSize = 8192;
config.asyncThreadCount = 4;
```

## Usage Examples

### Multiple Logger Instances

```cpp
const auto mainContext = kvalog::Logger::Context{ .appName = "MainService", .moduleName = "Core" };
auto mainLogger = kvalog::CreateLogger(kvalog::LogProfile::ColoredDefault, mainContext);

// Create subsystem logger with inherited config
const auto dbContext = kvalog::Logger::Context{ .appName = "MainService", .moduleName = "Database" };
auto dbLogger = kvalog::Logger::WithConfigFrom(*mainLogger, dbContext);

mainLogger->Info("Application started");
dbLogger->Info("Database connected");
```

### Setting Log Levels

```cpp
auto logger = kvalog::CreateLogger(kvalog::LogProfile::Default, context);

logger->SetLevel(kvalog::LogLevel::Warning);

// These won't be logged
logger->Trace("Not shown");
logger->Debug("Not shown");
logger->Info("Not shown");

// These will be logged
logger->Warning("Shown");
logger->Error("Shown");
logger->Critical("Shown");
```

### Runtime Field Configuration

```cpp
auto logger = kvalog::CreateLogger(kvalog::LogProfile::Verbose, context);
logger->Info("With all fields");

// Disable some fields at runtime
auto minimal = kvalog::LogFieldConfig();
minimal.includeProcessId = false;
minimal.includeThreadId = false;
minimal.includeFile = false;

logger->SetFieldConfig(minimal);
logger->Info("With minimal fields");
```

### Multi-threaded Async Logging

```cpp
auto config = kvalog::MakeProfileConfig(kvalog::LogProfile::Verbose);
config.asyncMode = kvalog::Logger::Mode::Async;
config.asyncQueueSize = 16384;
config.asyncThreadCount = 2;

const auto context = kvalog::Logger::Context{ .appName = "WorkerService", .moduleName = "Pool" };
auto logger = kvalog::Logger::Create(config, context);

auto workers = std::vector<std::thread>();
for (int i = 0; i < 10; ++i) {
    workers.emplace_back([&logger, i]() {
        for (int j = 0; j < 1000; ++j) {
            logger->Info("Worker {} processing item {}", i, j);
        }
    });
}

for (auto & worker : workers) {
    worker.join();
}

logger->Flush();
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
auto logger = kvalog::CreateLogger(kvalog::LogProfile::Default, context);

auto t1 = std::thread([&]() { logger->Info("Message from thread 1"); });
auto t2 = std::thread([&]() { logger->Info("Message from thread 2"); });

t1.join();
t2.join();
```

## Best Practices

1. **Use profiles**: Start with a profile and customize only if needed
2. **Module naming**: Use consistent module names across your codebase
3. **Config reuse**: Create logger instances with `WithConfigFrom()` for consistency
4. **Async for production**: Use async mode in production for better performance
5. **Flush on shutdown**: Always call `Flush()` before application exit
6. **Network error handling**: Implement robust error handling in network adapters
7. **Log levels**: Use appropriate log levels (don't log everything as Info)

## API Reference

### LogProfile

```cpp
enum class LogProfile {
    Minimal,          // App + level + message
    Default,          // + module + file
    Detailed,         // + timestamps
    Verbose,          // + PID/TID
    ColoredDefault,   // Default with colors
    ColoredDetailed,  // Detailed with colors
    ColoredVerbose,   // Verbose with colors
    Json              // Structured JSON output
};
```

### Logger::Config

```cpp
struct Config {
    OutputFormat format;                          // Json or Terminal
    LogFieldConfig fields;                        // Field configuration
    Mode asyncMode;                               // Sync or Async
    bool logToConsole;                            // Enable console output
    bool enableColors;                            // Enable colored level tags (terminal only)
    std::optional<std::string> logFilePath;       // File path (optional)
    std::shared_ptr<INetworkSink> networkAdapter; // Network adapter (optional)
    std::size_t asyncQueueSize;                   // Async queue size
    std::size_t asyncThreadCount;                 // Async thread count
};
```

### Logger Methods

```cpp
// Logging (accept fmt-style format strings)
void Trace(FormatString format, Args &&... args);
void Debug(FormatString format, Args &&... args);
void Info(FormatString format, Args &&... args);
void Warning(FormatString format, Args &&... args);
void Error(FormatString format, Args &&... args);
void Critical(FormatString format, Args &&... args);

// Configuration
void SetLevel(LogLevel level);
void SetFieldConfig(const LogFieldConfig & fields);
void SetOutputFormat(OutputFormat format);
void Flush();

// Fabric methods
static LoggerPtr Create(const Config & config);
static LoggerPtr Create(const Config & config, const Context & context);
static Logger WithConfigFrom(const Logger & source, const Context & newContext);
```

### Free Functions

```cpp
// Create logger from a profile
LoggerPtr CreateLogger(LogProfile profile, const Logger::Context & context);

// Get a profile's config for further customization
Logger::Config MakeProfileConfig(LogProfile profile);
```

## Summary

This is a customized way to use logging in our applications. We suggest unified interface to different logging targets. Main work of implementation was done by contrubutors of libraries [spdlog](https://github.com/gabime/spdlog) and [nlohmann/json](https://github.com/nlohmann/json).
