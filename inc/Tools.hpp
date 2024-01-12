#ifndef __TOOLS__BK
#define __TOOLS__BK

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <array>

#include <cmath>
#include <cstdarg>
#include <chrono>
#include <cstdio>
#include <ctime>
using t_string = std::string;
namespace fs = std::filesystem;
/*********** Class Usage **********
>>>>>>>>>>>>>>>>>>>>  Logger

    Logger::setLogLevel(Logger::CommandLine::parseLogLevel(argc, argv));
    Logger::initializeLogger();   // 启用日志文件写入
    //Logger::initializeLogger(false); // 不启用

    LOG_INFO("hello world");
    LOG_ERROR("This is an error message.");
    LOG_WARNING("This is a warning message.");
    LOG_INFO("This is an info message.");
    LOG_DEBUG("This is a debug message.");

    Logger::shutdownLogger();     //  关闭日志文件写入

    LOG_ERROR("Error: %d - %f", 42, 3.14);
    LOG_DEBUG("Debug: Value is %d", 42);

<<<<<<<<<<<<<<<<<<<<  Logger
>>>>>>>>>>>>>>>>>>>>  Config

    beiklive::CONFIG::Config myConfig("config.ini");

    // 设置配置项
    myConfig.setConfig("Network", "Port", "8080");
    myConfig.setConfig("General", "Username", "JohnDoe");

    // 打印指定分类下的所有配置项
    std::cout << "Initial Configuration:\n";
    myConfig.printAllConfigs("Network");
    myConfig.printAllConfigs("General");

    // 修改配置项
    myConfig.setConfig("Network", "Port", "9090");
    myConfig.setConfig("General", "Username", "JaneDoe");

    // 打印修改后的配置项
    std::cout << "\nModified Configuration:\n";
    myConfig.printAllConfigs("Network");
    myConfig.printAllConfigs("General");



<<<<<<<<<<<<<<<<<<<<  Config



***********************************/

/*           Signal And Slot              */
namespace
{
/*
>>>>> Init
    D_SIGNAL(onValueChanged, int);
    D_SLOT(showNumber, int value);
    void showNumber(int value)
    {
        LOG_INFO("Number : %d", value);
    }
>>>>> main
    连接信号和槽
    connect_onValueChanged(showNumber);

    发射信号
    emit_onValueChanged(42);


*/

// 定义一个宏来简化信号和槽的声明，支持参数
#define D_SIGNAL(name, ...)                                                            \
    std::function<void(__VA_ARGS__)> name;                                             \
    void connect_##name(const std::function<void(__VA_ARGS__)> &slot) { name = slot; } \
    void emit_##name(__VA_ARGS__ args)                                                 \
    {                                                                                  \
        if (name)                                                                      \
            name(args);                                                                \
    }

// 定义一个宏来简化槽函数的声明，支持参数
#define D_SLOT(name, ...) void name(__VA_ARGS__)
}

namespace beiklive
{
    namespace TOOL
    {

        /// 获取当前路径    /xxxx/xxxx/dir
        t_string GetPwd()
        {
            std::filesystem::path currentPath = std::filesystem::current_path();
            return currentPath;
        }
        /// 获取文件内容
        t_string readFileToString(const t_string &filename)
        {
            // 打开文件
            std::ifstream file(filename);

            // 检查文件是否成功打开
            if (!file.is_open())
            {
                std::cerr << "Error opening file: " << filename << std::endl;
                return "";
            }

            // 使用流操作符读取文件内容到字符串流
            std::stringstream buffer;
            buffer << file.rdbuf();

            // 关闭文件
            file.close();

            // 返回读取到的字符串
            return buffer.str();
        }
        /// 获取指定目录下所有文件名称
        std::vector<t_string> listFilesInDirectory(const t_string &directoryPath)
        {
            std::vector<t_string> filenames;

            for (const auto &entry : std::filesystem::directory_iterator(directoryPath))
            {
                filenames.push_back(entry.path().filename().string());
            }

            return filenames;
        }
        /// 目录不存在则创建
        bool createDirectoryIfNotExists(const t_string &directoryPath)
        {
            if (!std::filesystem::exists(directoryPath))
            {
                try
                {
                    std::filesystem::create_directory(directoryPath);
                    return true;
                }
                catch (const std::exception &e)
                {
                    return false;
                }
            }
            return true; // Directory already exists
        }

        // 执行系统命令
        std::string executeCommand(const std::string &command)
        {
            // 打开进程管道
            FILE *pipe = popen(command.c_str(), "r");
            if (!pipe)
            {
                throw std::runtime_error("popen() failed!");
            }

            // 读取命令执行结果
            std::array<char, 128> buffer;
            std::string result;

            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
            {
                result += buffer.data();
            }

            // 关闭管道
            int status = pclose(pipe);

            // 检查命令执行结果
            if (status != 0)
            {
                throw std::runtime_error("Command execution failed!");
            }

            return result;
        }
        bool isFileModified(const fs::path &filePath, const fs::file_time_type &lastCheckTime)
        {
            try
            {
                auto lastWriteTime = fs::last_write_time(filePath);
                return lastWriteTime > lastCheckTime;
            }
            catch (const std::exception &e)
            {
                // Handle exception (file not found, permissions issue, etc.)
                std::cerr << "Error: " << e.what() << std::endl;
                return false;
            }
        }

        void checkDirectory(const fs::path &directoryPath, const fs::file_time_type &lastCheckTime)
        {
            for (const auto &entry : fs::recursive_directory_iterator(directoryPath))
            {
                if (fs::is_regular_file(entry))
                {
                    if (isFileModified(entry.path(), lastCheckTime))
                    {
                        std::cout << entry.path().string() << " has been modified!" << std::endl;
                    }
                }
            }
        }

    }

    namespace Logger
    {
        enum LogLevel
        {
            ERROR,
            WARNING,
            INFO,
            DEBUG
        };

        namespace
        {
            LogLevel logLevel = LogLevel::INFO;
            std::ofstream logFile;
            std::mutex logMutex;
            bool logFileEnabled = true;
            bool logConsoleOutputEnable = true;
            t_string logFileNamePrefix = "./log/log";
            t_string logFileExtension = ".log";
            unsigned long maxLogFileSize = 50 * 1024 * 1024; // 50 MB
            unsigned int logFileIndex = 0;
            unsigned int logFileCount = 0;
        }
        t_string getTimestamp()
        {
            auto now = std::chrono::system_clock::now();
            auto time_t_now = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

            std::ostringstream oss;

#ifdef _WIN32
            std::tm tm = {};
            localtime_s(&tm, &time_t_now);
            char timestamp[32];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);
            oss << timestamp << "." << ms.count();
#else
            std::tm tm;
            localtime_r(&time_t_now, &tm);
            char timestamp[32];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);
            oss << timestamp << "." << ms.count();
#endif

            return oss.str();
        }
        void createNewLogFile()
        {
            std::lock_guard<std::mutex> lock(logMutex);
            if (logFile.is_open())
            {
                logFile.close();
            }

            std::ostringstream logFileName;
            logFileName << logFileNamePrefix << "_" << getTimestamp() << "_" << logFileIndex << logFileExtension;
            logFile.open(logFileName.str(), std::ios::app);

            if (logFile.is_open())
            {
                std::cout << "New log file created: " << logFileName.str() << std::endl;
                logFileCount++;
            }
            else
            {
                std::cerr << "Unable to open log file: " << logFileName.str() << std::endl;
            }
        }

        void initializeLogger(bool enableLogFile = true)
        {
            logFileEnabled = enableLogFile;

            if (logFileEnabled)
            {
                createNewLogFile();
            }
        }

        void ConsoleOutputCtrl(bool enable = true)
        {
            logConsoleOutputEnable = enable;
        }

        void shutdownLogger()
        {
            std::lock_guard<std::mutex> lock(logMutex);
            if (logFile.is_open())
            {
                logFile.close();
            }
        }

        void setLogLevel(LogLevel level)
        {
            logLevel = level;
        }

        t_string getLogLevelString(LogLevel level)
        {
            switch (level)
            {
            case LogLevel::ERROR:
                return "\033[1;31mERROR\033[0m"; // 红色
            case LogLevel::WARNING:
                return "\033[1;33mWARNING\033[0m"; // 黄色
            case LogLevel::INFO:
                return "\033[1;32mINFO\033[0m"; // 绿色
            case LogLevel::DEBUG:
                return "\033[1;34mDEBUG\033[0m"; // 蓝色
            default:
                return "UNKNOWN";
            }
        }
        t_string getLogLevelStringNoColor(LogLevel level)
        {
            switch (level)
            {
            case LogLevel::ERROR:
                return "E";
            case LogLevel::WARNING:
                return "W";
            case LogLevel::INFO:
                return "I";
            case LogLevel::DEBUG:
                return "D";
            default:
                return "UNKNOWN";
            }
        }
        template <typename T>
        t_string formatLogMessage(const T &value)
        {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }

        template <typename T, typename... Args>
        t_string formatLogMessage(const T &value, const Args &...args)
        {
            return formatLogMessage(value) + formatLogMessage(args...);
        }

        void log(LogLevel level, const char *format, ...)
        {
            if (level <= logLevel)
            {
                std::lock_guard<std::mutex> lock(logMutex);

                va_list args;
                va_start(args, format);

                char buffer[256];
                vsnprintf(buffer, sizeof(buffer), format, args);

                va_end(args);

                if (logConsoleOutputEnable)
                {
                    std::cout << "[" << getTimestamp() << "] [" << getLogLevelString(level) << "] " << buffer << std::endl;
                }

                if (logFileEnabled && logFile.is_open())
                {
                    logFile << "[" << getTimestamp() << "] [" << getLogLevelStringNoColor(level) << "] " << buffer << std::endl;
                    logFile.flush();

                    // Check if log file size exceeds the maximum limit
                    if (logFile.tellp() >= maxLogFileSize)
                    {
                        logFileIndex++;
                        createNewLogFile();
                    }
                }
            }
        }

        namespace CommandLine
        {
            LogLevel parseLogLevel(int argc, char *argv[])
            {
                LogLevel level = LogLevel::INFO;

                for (int i = 1; i < argc; ++i)
                {
                    t_string arg = argv[i];

                    if (arg == "--log-level" && i + 1 < argc)
                    {
                        t_string levelStr = argv[i + 1];

                        if (levelStr == "ERROR")
                        {
                            level = LogLevel::ERROR;
                        }
                        else if (levelStr == "WARNING")
                        {
                            level = LogLevel::WARNING;
                        }
                        else if (levelStr == "INFO")
                        {
                            level = LogLevel::INFO;
                        }
                        else if (levelStr == "DEBUG")
                        {
                            level = LogLevel::DEBUG;
                        }
                        else
                        {
                            std::cerr << "Invalid log level specified. Using default level (INFO)." << std::endl;
                        }
                    }
                }

                return level;
            }
        }
    }

#define LOG_ERROR(...) beiklive::Logger::log(beiklive::Logger::LogLevel::ERROR, __VA_ARGS__)
#define LOG_WARNING(...) beiklive::Logger::log(beiklive::Logger::LogLevel::WARNING, __VA_ARGS__)
#define LOG_INFO(...) beiklive::Logger::log(beiklive::Logger::LogLevel::INFO, __VA_ARGS__)
#define LOG_DEBUG(...) beiklive::Logger::log(beiklive::Logger::LogLevel::DEBUG, __VA_ARGS__)

    namespace Coord
    {
        // Point
        template <class A>
        class Point
        {
        public:
            Point() = default;
            ~Point() = default;

            Point(A x, A y) : _x(x), _y(y) {}

            A getX() const { return _x; }
            A getY() const { return _y; }

            bool operator==(const Point &p) const
            {
                return (_x == p.getX()) && (_y == p.getY());
            }

            template <class B>
            Point &operator*=(const B &coefficient)
            {
                _x *= coefficient;
                _y *= coefficient;
                return *this;
            }

            template <class B>
            Point &operator/=(const B &coefficient)
            {
                if (coefficient != 0)
                {
                    _x /= coefficient;
                    _y /= coefficient;
                }
                return *this;
            }

            Point &operator+=(const Point &p)
            {
                _x += p.getX();
                _y += p.getY();
                return *this;
            }

            Point &operator-=(const Point &p)
            {
                _x -= p.getX();
                _y -= p.getY();
                return *this;
            }

            friend Point operator+(const Point &p1, const Point &p2)
            {
                return Point(p1.getX() + p2.getX(), p1.getY() + p2.getY());
            }

            friend Point operator-(const Point &p1, const Point &p2)
            {
                return Point(p1.getX() - p2.getX(), p1.getY() - p2.getY());
            }

            template <class B>
            friend Point operator*(const Point &p, const B &coefficient)
            {
                return Point(p.getX() * coefficient, p.getY() * coefficient);
            }

            template <class B>
            friend Point operator*(const B &coefficient, const Point &p)
            {
                return p * coefficient;
            }

            template <class B>
            friend Point operator/(const Point &p, const B &coefficient)
            {
                Point pp(p);
                pp /= coefficient;
                return pp;
            }

            double distanceTo(const Point &other) const
            {
                double dx = _x - other.getX();
                double dy = _y - other.getY();
                return std::sqrt(dx * dx + dy * dy);
            }

            double angleTo(const Point &other) const
            {
                double dx = other.getX() - _x;
                double dy = other.getY() - _y;
                return std::atan2(dy, dx);
            }

            Point midpoint(const Point &other) const
            {
                return Point((_x + other.getX()) / 2, (_y + other.getY()) / 2);
            }

            bool isInsideRectangle(const Point &topLeft, const Point &bottomRight) const
            {
                return (_x >= topLeft.getX() && _x <= bottomRight.getX() &&
                        _y >= topLeft.getY() && _y <= bottomRight.getY());
            }

            void rotate(double angleInRadians)
            {
                double newX = _x * std::cos(angleInRadians) - _y * std::sin(angleInRadians);
                double newY = _x * std::sin(angleInRadians) + _y * std::cos(angleInRadians);
                _x = newX;
                _y = newY;
            }

        private:
            A _x;
            A _y;
        };
    }

    namespace CONFIG
    {
        class Config
        {
        private:
            std::unordered_map<t_string, std::unordered_map<t_string, t_string>> configMap;
            t_string _filename;

        public:
            Config() = default;
            Config(const t_string &filename)
            {
                _filename = filename;

                LOG_DEBUG("[%s] config file name : %s", __func__, _filename.c_str());
            }

            // 设置配置项
            void setConfig(const t_string &category, const t_string &key, const t_string &value)
            {
                configMap[category][key] = value;
                LOG_DEBUG("[%s] %s::%s : %s", __func__, category.c_str(), key.c_str(), value.c_str());
                saveToFile(_filename);
            }
            // void setConfig(const t_string &category, const t_string &key, const int &value)
            // {
            //     configMap[category][key] = std::to_string(value);
            //     LOG_DEBUG("[%s] %s::%s : %d", __func__, category.c_str(), key.c_str(), value);
            //     saveToFile(_filename);
            // }
            // 获取配置项
            t_string getConfig(const t_string &category, const t_string &key) const
            {
                auto categoryIt = configMap.find(category);
                if (categoryIt != configMap.end())
                {
                    auto keyIt = categoryIt->second.find(key);
                    if (keyIt != categoryIt->second.end())
                    {
                        LOG_DEBUG("[%s] %s::%s : %s", __func__, category.c_str(), key.c_str(), keyIt->second.c_str());
                        return keyIt->second;
                    }
                }
                // 返回空字符串表示配置项不存在
                LOG_ERROR("[%s] %s::%s not found", __func__, category.c_str(), key.c_str());
                return "";
            }

            // 删除配置项
            void removeConfig(const t_string &category, const t_string &key)
            {
                auto categoryIt = configMap.find(category);
                if (categoryIt != configMap.end())
                {
                    categoryIt->second.erase(key);
                }
                saveToFile(_filename);
            }

            // 打印指定分类下的所有配置项
            void printAllConfigs(const t_string &category) const
            {
                auto categoryIt = configMap.find(category);
                if (categoryIt != configMap.end())
                {
                    for (const auto &pair : categoryIt->second)
                    {
                        LOG_DEBUG("[%s] %s : %s", __func__, pair.first.c_str(), pair.second.c_str());
                    }
                }
            }

            // 保存配置到文件
            bool saveToFile(const t_string &filename) const
            {
                std::ofstream file(filename);
                if (file.is_open())
                {
                    for (const auto &categoryPair : configMap)
                    {
                        file << "[" << categoryPair.first << "]\n";
                        for (const auto &pair : categoryPair.second)
                        {
                            file << pair.first << "=" << pair.second << std::endl;
                        }
                        file << "\n"; // 每个块之间添加空行
                    }
                    file.close();
                    LOG_DEBUG("[%s] Configuration saved to %s", __func__, filename.c_str());
                    return true;
                }
                else
                {
                    LOG_ERROR("[%s] Unable to open file: %s", __func__, filename.c_str());
                    return false;
                }
            }

            // 从文件加载配置
            bool loadFromFile()
            {
                std::ifstream file(_filename);
                if (file.is_open())
                {
                    t_string line;
                    t_string currentCategory;

                    while (std::getline(file, line))
                    {
                        size_t bracketOpenPos = line.find('[');
                        size_t bracketClosePos = line.find(']');

                        if (bracketOpenPos != t_string::npos && bracketClosePos != t_string::npos)
                        {
                            currentCategory = line.substr(bracketOpenPos + 1, bracketClosePos - bracketOpenPos - 1);
                        }
                        else
                        {
                            size_t delimiterPos = line.find('=');
                            if (delimiterPos != t_string::npos && !currentCategory.empty())
                            {
                                t_string key = line.substr(0, delimiterPos);
                                t_string value = line.substr(delimiterPos + 1);
                                configMap[currentCategory][key] = value;
                                LOG_DEBUG("[%s] load %s::%s : %s", __func__, currentCategory.c_str(), key.c_str(), value.c_str());
                            }
                        }
                    }

                    file.close();
                    LOG_DEBUG("[%s] Configuration loaded from %s success", __func__, _filename.c_str());
                    return true;
                }
                else
                {
                    LOG_DEBUG("[%s] Unable to open file: %s", __func__, _filename.c_str());
                    return false;
                }
            }
        };
    }

    namespace StateMachine
    {
        class StateMachine
        {
        public:
            StateMachine() : currentStateName("") {}

            void addState(const t_string &name, const std::function<void()> &stateFunction)
            {
                states[name] = stateFunction;
            }

            void addTransition(const t_string &from, const t_string &to, const std::function<bool()> &transitionFunction)
            {
                transitions[from][to] = transitionFunction;
            }

            void start(const t_string &initialState)
            {
                currentStateName = initialState;
                if (states.find(currentStateName) != states.end())
                {
                    states[currentStateName]();
                }
                else
                {
                    std::cerr << "Invalid initial state: " << initialState << std::endl;
                }
            }

            void triggerTransition()
            {
                if (transitions.find(currentStateName) != transitions.end())
                {
                    for (const auto &entry : transitions[currentStateName])
                    {
                        if (entry.second())
                        {
                            nextStateName = entry.first;
                            return;
                        }
                    }
                }

                nextStateName = ""; // No valid transition found
            }

            void updateState()
            {
                if (!nextStateName.empty())
                {
                    currentStateName = nextStateName;
                    if (states.find(currentStateName) != states.end())
                    {
                        states[currentStateName]();
                    }
                    else
                    {
                        std::cerr << "Invalid state: " << currentStateName << std::endl;
                    }
                    nextStateName = "";
                }
            }

            t_string getCurrentState() const
            {
                return currentStateName;
            }
            t_string getNextState() const
            {
                return nextStateName;
            }

        private:
            std::unordered_map<t_string, std::function<void()>> states;
            std::unordered_map<t_string, std::unordered_map<t_string, std::function<bool()>>> transitions;
            t_string currentStateName;
            t_string nextStateName;
        };
    } // namespace StateMachine

} // namespace beiklive

#endif // !__TOOLS__BK
