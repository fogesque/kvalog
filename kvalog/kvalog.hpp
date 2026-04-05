#pragma once

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <source_location>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace kvalog
{

// Forward declarations
class INetworkSink;
class NetworkSink;
class Logger;

// Type aliases
using LoggerPtr = std::shared_ptr<Logger>;

/// @brief Log levels matching spdlog
enum class LogLevel {
    Off,
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

/// @brief Configuration for which fields to include in logs
struct LogFieldConfig {
    bool includeAppName = true;
    bool includeProcessId = true;
    bool includeThreadId = true;
    bool includeModuleName = true;
    bool includeLogLevel = true;
    bool includeFile = true;
    bool includeMessage = true;
    bool includeTime = true;
};

/// @brief Output format type
enum class OutputFormat {
    Json,
    Terminal
};

///
/// @brief
/// INetworkSink defines the interface for network sink adapters
///
class INetworkSink
{
public:
    /// @brief Destructor
    virtual ~INetworkSink() = default;

    /// @brief Sends a formatted log entry over the network
    virtual void SendLog(const std::string & jsonLog) = 0;
    /// @brief Returns whether the network connection is active
    virtual bool IsConnected() const = 0;
};

///
/// @brief
/// NetworkSink is a custom spdlog sink that forwards log messages to a network adapter
///
class NetworkSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
    /// [Construction & Destruction]

#pragma region NetworkSink::Construct

    /// @brief Constructor with network adapter
    explicit NetworkSink(std::shared_ptr<INetworkSink> initialAdapter)
        : adapter(std::move(initialAdapter))
    {
    }

#pragma endregion

    /// [Adapter Management]

    /// @brief Replaces the current network adapter with a new one
    void SetAdapter(std::shared_ptr<INetworkSink> newAdapter)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        this->adapter = std::move(newAdapter);
    }

protected:
    /// @brief Formats and sends a log message through the network adapter
    void sink_it_(const spdlog::details::log_msg & message) override
    {
        if (this->adapter && this->adapter->IsConnected()) {
            spdlog::memory_buf_t formatted;
            this->formatter_->format(message, formatted);
            this->adapter->SendLog(fmt::to_string(formatted));
        }
    }

    /// @brief Flushes the sink (no-op for network sinks)
    void flush_() override {}

private:
    /// [Properties]

    /// @brief Network adapter for sending log messages
    std::shared_ptr<INetworkSink> adapter = nullptr;
};

/// @brief Default async queue size
inline constexpr std::size_t DefaultAsyncQueueSize = 8192;
/// @brief Default async thread count
inline constexpr std::size_t DefaultAsyncThreadCount = 1;
/// @brief Milliseconds divisor for time formatting
inline constexpr int MillisecondsDivisor = 1000;
/// @brief Millisecond field width for time formatting
inline constexpr int MillisecondsFieldWidth = 3;

/// @brief ANSI escape codes for terminal log coloring
namespace AnsiColor
{
/// @brief Resets all terminal formatting
inline constexpr auto Reset = "\033[0m";
/// @brief Warm tint for the entire log line (light yellow foreground)
inline constexpr auto WarmTint = "\033[38;5;223m";
/// @brief Trace level color (light velvet / light magenta)
inline constexpr auto Trace = "\033[38;5;183m";
/// @brief Debug level color (light cyan)
inline constexpr auto Debug = "\033[96m";
/// @brief Info level color (light blue)
inline constexpr auto Info = "\033[94m";
/// @brief Warning level color (yellow)
inline constexpr auto Warning = "\033[33m";
/// @brief Error level color (red)
inline constexpr auto Error = "\033[31m";
/// @brief Critical level color (bold red)
inline constexpr auto Critical = "\033[1;31m";
}  // namespace AnsiColor

///
/// @brief
/// Logger provides structured logging with configurable output formats, sinks, and fields
///
class Logger
{
public:
    /// @brief Logging mode
    enum class Mode {
        Sync,
        Async
    };

    /// @brief Logger configuration
    struct Config {
        OutputFormat format = OutputFormat::Terminal;
        LogFieldConfig fields = LogFieldConfig();
        Mode asyncMode = Mode::Sync;

        bool logToConsole = true;
        bool enableColors = false;
        std::optional<std::string> logFilePath = std::nullopt;
        std::shared_ptr<INetworkSink> networkAdapter = nullptr;

        std::size_t asyncQueueSize = DefaultAsyncQueueSize;
        std::size_t asyncThreadCount = DefaultAsyncThreadCount;
    };

    /// @brief Context information for logs
    struct Context {
        std::string appName;
        std::string moduleName;
    };

    /// [Fabric Methods]

    /// @brief Creates a Logger instance with given configuration
    static LoggerPtr Create(const Config & config)
    {
        return std::make_shared<Logger>(config);
    }

    /// @brief Creates a Logger instance with given configuration and context
    static LoggerPtr Create(const Config & config, const Context & context)
    {
        return std::make_shared<Logger>(config, context);
    }

    /// @brief Creates a new Logger with configuration copied from an existing one
    static Logger WithConfigFrom(const Logger & source, const Context & newContext)
    {
        const auto newConfig = source.config;
        return Logger(newConfig, newContext);
    }

    /// [Field Configuration]

    /// @brief Updates field configuration at runtime
    void SetFieldConfig(const LogFieldConfig & fields)
    {
        this->config.fields = fields;
    }

    /// @brief Returns current field configuration
    LogFieldConfig GetFieldConfig() const
    {
        return this->config.fields;
    }

    /// @brief Updates output format at runtime
    void SetOutputFormat(OutputFormat format)
    {
        this->config.format = format;
    }

    /// [Logging]

    /// @brief Logs a message at Trace level
    void Trace(const std::string & message,
               const std::source_location & location = std::source_location::current())
    {
        this->log(LogLevel::Trace, message, location);
    }

    /// @brief Logs a message at Debug level
    void Debug(const std::string & message,
               const std::source_location & location = std::source_location::current())
    {
        this->log(LogLevel::Debug, message, location);
    }

    /// @brief Logs a message at Info level
    void Info(const std::string & message,
              const std::source_location & location = std::source_location::current())
    {
        this->log(LogLevel::Info, message, location);
    }

    /// @brief Logs a message at Warning level
    void Warning(const std::string & message,
                 const std::source_location & location = std::source_location::current())
    {
        this->log(LogLevel::Warning, message, location);
    }

    /// @brief Logs a message at Error level
    void Error(const std::string & message,
               const std::source_location & location = std::source_location::current())
    {
        this->log(LogLevel::Error, message, location);
    }

    /// @brief Logs a message at Critical level
    void Critical(const std::string & message,
                  const std::source_location & location = std::source_location::current())
    {
        this->log(LogLevel::Critical, message, location);
    }

    /// [Level Management]

    /// @brief Sets minimum log level
    void SetLevel(LogLevel level)
    {
        if (this->logger) {
            this->level = level;
            this->logger->set_level(Logger::toSpdlogLevel(level));
        }
    }

    /// @brief Flushes all pending log messages
    void Flush()
    {
        if (this->logger) {
            this->logger->flush();
        }
    }

    /// [Construction & Destruction]

#pragma region Logger::Construct

    /// @brief Copy constructor is deleted
    Logger(const Logger &) = delete;
    /// @brief Copy operator is deleted
    Logger & operator=(const Logger &) = delete;

    /// @brief Move constructor
    Logger(Logger &&) = default;
    /// @brief Move assignment operator
    Logger & operator=(Logger &&) = default;

    /// @brief Constructor with configuration
    /// @warning Avoid using this constructor since class has static fabric methods
    explicit Logger(const Config & initialConfig)
        : config(initialConfig), processId(Logger::getProcessId())
    {
        this->initializeLogger();
    }

    /// @brief Constructor with configuration and context
    /// @warning Avoid using this constructor since class has static fabric methods
    explicit Logger(const Config & initialConfig, const Context & initialContext)
        : config(initialConfig), context(initialContext), processId(Logger::getProcessId())
    {
        this->initializeLogger();
    }

    /// @brief Destructor
    ~Logger() = default;

#pragma endregion

private:
#pragma region Logger::PrivateMethods

    /// [Initialization]

    /// @brief Initializes the spdlog logger with configured sinks
    void initializeLogger()
    {
        auto sinks = std::vector<spdlog::sink_ptr>();

        if (this->config.logToConsole) {
            auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            consoleSink->set_pattern("%v");
            sinks.push_back(consoleSink);
        }

        if (this->config.logFilePath) {
            auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                *this->config.logFilePath, true);
            fileSink->set_pattern("%v");
            sinks.push_back(fileSink);
        }

        if (this->config.networkAdapter) {
            auto networkSink = std::make_shared<NetworkSink>(this->config.networkAdapter);
            networkSink->set_pattern("%v");
            sinks.push_back(networkSink);
        }

        if (this->config.asyncMode == Mode::Async) {
            spdlog::init_thread_pool(this->config.asyncQueueSize, this->config.asyncThreadCount);
            this->logger = std::make_shared<spdlog::async_logger>(
                "async_logger", sinks.begin(), sinks.end(), spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);
        } else {
            this->logger =
                std::make_shared<spdlog::logger>("sync_logger", sinks.begin(), sinks.end());
        }

        this->level = LogLevel::Trace;
        this->logger->set_level(spdlog::level::trace);
    }

    /// [Logging Implementation]

    /// @brief Formats and dispatches a log message at the given level
    void log(LogLevel level, const std::string & message, const std::source_location & location)
    {
        if (static_cast<int>(this->level) > static_cast<int>(level)) {
            return;
        }

        if (!this->logger) {
            return;
        }

        auto formattedMessage = std::string();

        if (this->config.format == OutputFormat::Json) {
            formattedMessage = this->formatJson(level, message, location);
        } else {
            formattedMessage = this->formatTerminal(level, message, location);
        }

        this->logger->log(Logger::toSpdlogLevel(level), formattedMessage);
    }

    /// [Formatting]

    /// @brief Formats a log entry as a JSON string
    std::string formatJson(LogLevel level, const std::string & message,
                           const std::source_location & location)
    {
        auto logEntry = nlohmann::json();

        if (this->config.fields.includeTime) {
            logEntry["time"] = Logger::getCurrentTimeString();
        }
        if (this->config.fields.includeAppName) {
            if (!this->context.appName.empty()) {
                logEntry["app"] = this->context.appName;
            }
        }
        if (this->config.fields.includeProcessId) {
            logEntry["process_id"] = std::to_string(this->processId);
        }
        if (this->config.fields.includeThreadId) {
            logEntry["thread_id"] = Logger::getThreadId();
        }
        if (this->config.fields.includeModuleName) {
            if (!this->context.moduleName.empty()) {
                logEntry["module"] = this->context.moduleName;
            }
        }
        if (this->config.fields.includeLogLevel) {
            logEntry["level"] = Logger::levelToString(level);
        }
        if (this->config.fields.includeFile) {
            logEntry["file"] = Logger::formatFileLine(location);
        }
        if (this->config.fields.includeMessage) {
            logEntry["message"] = message;
        }

        return logEntry.dump();
    }

    /// @brief Formats a log entry as a human-readable terminal string
    std::string formatTerminal(LogLevel level, const std::string & message,
                               const std::source_location & location)
    {
        auto output = std::ostringstream();
        const auto colored = this->useColors();

        const auto section = [](const std::string & content) { return "[" + content + "]"; };

        if (colored) {
            output << AnsiColor::WarmTint;
        }

        if (this->config.fields.includeTime) {
            output << section(Logger::getCurrentTimeString());
        }
        if (this->config.fields.includeAppName) {
            if (!this->context.appName.empty()) {
                output << section(this->context.appName);
            }
        }
        if (this->config.fields.includeModuleName) {
            if (!this->context.moduleName.empty()) {
                output << section(this->context.moduleName);
            }
        }
        if (this->config.fields.includeProcessId) {
            output << section("PID:" + std::to_string(this->processId));
        }
        if (this->config.fields.includeThreadId) {
            output << section("TID:" + Logger::getThreadId());
        }
        if (this->config.fields.includeLogLevel) {
            const auto levelText = Logger::levelToString(level);
            if (colored) {
                output << "[" << Logger::levelToColor(level) << levelText << AnsiColor::Reset
                       << AnsiColor::WarmTint << "]";
            } else {
                output << section(levelText);
            }
        }
        if (this->config.fields.includeFile) {
            output << section(Logger::formatFileLine(location));
        }
        if (this->config.fields.includeMessage) {
            output << " " << message;
        }

        if (colored) {
            output << AnsiColor::Reset;
        }

        return output.str();
    }

    /// [Utility]

    /// @brief Returns whether coloring should be applied to terminal output
    bool useColors() const
    {
        return this->config.enableColors && this->config.format == OutputFormat::Terminal &&
               this->config.logToConsole;
    }

    /// @brief Returns the ANSI color escape code for a given log level
    static const char * levelToColor(LogLevel level)
    {
        switch (level) {
            case LogLevel::Trace:
                return AnsiColor::Trace;
            case LogLevel::Debug:
                return AnsiColor::Debug;
            case LogLevel::Info:
                return AnsiColor::Info;
            case LogLevel::Warning:
                return AnsiColor::Warning;
            case LogLevel::Error:
                return AnsiColor::Error;
            case LogLevel::Critical:
                return AnsiColor::Critical;
            default:
                return AnsiColor::Info;
        }
    }

    /// @brief Extracts filename and line number from a source location
    static std::string formatFileLine(const std::source_location & location)
    {
        auto file = std::string(location.file_name());
        const auto position = file.find_last_of("/\\");
        if (position != std::string::npos) {
            file = file.substr(position + 1);
        }

        auto fileLine = std::ostringstream();
        fileLine << file << ":" << location.line();
        return fileLine.str();
    }

    /// @brief Converts a LogLevel to its short string representation
    static std::string levelToString(LogLevel level)
    {
        switch (level) {
            case LogLevel::Trace:
                return "TRC";
            case LogLevel::Debug:
                return "DBG";
            case LogLevel::Info:
                return "INF";
            case LogLevel::Warning:
                return "WRN";
            case LogLevel::Error:
                return "ERR";
            case LogLevel::Critical:
                return "CRT";
            default:
                return "INF";
        }
    }

    /// @brief Converts a LogLevel to the corresponding spdlog level
    static spdlog::level::level_enum toSpdlogLevel(LogLevel level)
    {
        switch (level) {
            case LogLevel::Off:
                return spdlog::level::off;
            case LogLevel::Trace:
                return spdlog::level::trace;
            case LogLevel::Debug:
                return spdlog::level::debug;
            case LogLevel::Info:
                return spdlog::level::info;
            case LogLevel::Warning:
                return spdlog::level::warn;
            case LogLevel::Error:
                return spdlog::level::err;
            case LogLevel::Critical:
                return spdlog::level::critical;
            default:
                return spdlog::level::info;
        }
    }

    /// @brief Returns the current process ID
    static int getProcessId()
    {
#ifdef _WIN32
        return static_cast<int>(GetCurrentProcessId());
#else
        return static_cast<int>(getpid());
#endif
    }

    /// @brief Returns the current thread ID as a string
    static std::string getThreadId()
    {
        auto output = std::ostringstream();
        output << std::this_thread::get_id();
        return output.str();
    }

    /// @brief Returns the current timestamp as a formatted string
    static std::string getCurrentTimeString()
    {
        const auto now = std::chrono::system_clock::now();
        const auto milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) %
            MillisecondsDivisor;
        const auto timeValue = std::chrono::system_clock::to_time_t(now);

        auto output = std::ostringstream();
        output << std::put_time(std::localtime(&timeValue), "%Y-%m-%d %H:%M:%S");
        output << '.' << std::setfill('0') << std::setw(MillisecondsFieldWidth)
               << milliseconds.count();
        return output.str();
    }

#pragma endregion

    /// [Properties]

    /// @brief Current minimum log level
    LogLevel level = LogLevel::Trace;
    /// @brief Underlying spdlog logger instance
    std::shared_ptr<spdlog::logger> logger = nullptr;
    /// @brief Logger configuration
    Config config;
    /// @brief Logger context with app and module names
    Context context;
    /// @brief Cached process ID
    int processId = 0;
};

/// @brief Creates a default logger configuration for terminal output
inline Logger::Config MakeDefaultLoggerConfig()
{
    auto config = Logger::Config();
    config.format = OutputFormat::Terminal;
    config.fields = LogFieldConfig{ .includeAppName = true,
                                    .includeProcessId = false,
                                    .includeThreadId = false,
                                    .includeModuleName = true,
                                    .includeLogLevel = true,
                                    .includeFile = true,
                                    .includeMessage = true,
                                    .includeTime = false };
    config.asyncMode = Logger::Mode::Sync;
    config.logToConsole = true;
    config.logFilePath = std::nullopt;
    config.networkAdapter = nullptr;
    config.asyncQueueSize = DefaultAsyncQueueSize;
    config.asyncThreadCount = DefaultAsyncThreadCount;
    return config;
}

/// @brief Creates a Logger with default terminal configuration and given app/module names
inline LoggerPtr CreateLogger(const std::string & appName, const std::string & moduleName)
{
    const auto defaultConfig = MakeDefaultLoggerConfig();

    auto context = Logger::Context();
    context.appName = appName;
    context.moduleName = moduleName;

    return Logger::Create(defaultConfig, context);
}

}  // namespace kvalog
