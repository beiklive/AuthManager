#include <iostream>
#include "../inc/Tools.hpp"
#include "../inc/cpp-httplib/httplib.h"
#include "../inc/nlohmann/json.hpp"

/*  验证管理器功能
1. 基本的 Get Post请求, 基础网页能力
2. Api 接口能力， 以配置文件驱动
*/
using json = nlohmann::json;
using ConfigList = std::vector<json>;
enum
{
    FAILED,
    SUCCESS,
    ERROR
};
class AppConfigManager;
class AuthManager;
class GrammarCheck;
class RuleEngine;

class RuleEngine
{
public:
    RuleEngine() = default;

    bool loadRules()
    {
        ruleFileName = beiklive::TOOL::GetPwd() + "/../rules.json";
        try
        {
            std::ifstream file(ruleFileName);
            if (!file.is_open())
            {
                std::cerr << "Error opening file: " << ruleFileName << std::endl;
                return false;
            }

            ruleData = json::parse(file);

            file.close();
            return true;
        }
        catch (const json::parse_error &e)
        {
            std::cerr << "Error parsing JSON file: " << e.what() << std::endl;
            return false;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool applyRule(const json &jsonData) const
    {
        if (!ruleData.contains("rules") || !ruleData["rules"].is_array())
        {
            std::cerr << "Error: Missing or invalid rules in rule configuration." << std::endl;
            return false;
        }

        for (const auto &rule : ruleData["rules"])
        {
            if (rule.contains("syntax") && rule["syntax"].is_object())
            {
                bool match = true;
                for (const auto &[key, expectedType] : rule["syntax"].items())
                {
                    if (!jsonData.contains(key) || !matchType(jsonData[key], expectedType))
                    {
                        match = false;
                        break;
                    }
                }

                if (match)
                {
                    // std::cout << "Matched rule: " << rule["name"] << std::endl;
                    return true;
                }
            }
        }

        // std::cerr << "No matching rule found." << std::endl;
        return false;
    }

    std::vector<std::string> getRuleListByTypeStr(const std::string &typeName) const
    {
        std::vector<std::string> SyntaxList;
        if (!ruleData.contains("rules") || !ruleData["rules"].is_array())
        {
            std::cerr << "Error: Missing or invalid rules in rule configuration." << std::endl;
            return SyntaxList;
        }

        // find target type
        bool match = false;
        for (const auto &rule : ruleData["rules"])
        {
            if (rule.contains("type") && rule["type"].is_string())
            {
                if (typeName == rule["type"])
                {
                    LOG_DEBUG("[%s] type: %s", __func__, typeName.c_str());
                    for (const auto &[key, expectedType] : rule["syntax"].items())
                    {
                        LOG_DEBUG("[%s] key: %s", __func__, key.c_str());
                        SyntaxList.emplace_back(key);
                    }
                    match = true;
                    break;
                }
            }
        }
        return SyntaxList;
    }

private:
    bool matchType(const json &value, const std::string &expectedType) const
    {
        if (expectedType == "string")
        {
            return value.is_string();
        }
        else if (expectedType == "number")
        {
            return value.is_number();
        }
        else if (expectedType == "boolean")
        {
            return value.is_boolean();
        }
        else
        {
            std::cerr << "Unknown type in rule: " << expectedType << std::endl;
            return false;
        }
    }

private:
    std::string ruleFileName;
    json ruleData;
};

class GrammarCheck
{
private:
    RuleEngine ruleEngine;
    ConfigList _configList;
    std::string _syntaxErrorMessage{"NONE"};
    std::string _routerErrorMessage{"NONE"};

public:
    GrammarCheck()
    {
        ruleEngine.loadRules();
    }
    ~GrammarCheck() = default;

    /// 添加需要 check 的内容
    void configAdd(const json &jsonp)
    {
        _configList.emplace_back(jsonp);
    }

    /// 清空 待check 列表
    void configClean()
    {
        _configList.clear();
        _syntaxErrorMessage = "NONE";
        _routerErrorMessage = "NONE";
    }

    void syntaxErrorMessageAdd(const std::string &msg)
    {
        if ("NONE" == _syntaxErrorMessage)
        {
            _syntaxErrorMessage = "Syntax error found!!! Please check the following configuration >>";
        }
        _syntaxErrorMessage += "\n";
        _syntaxErrorMessage += msg;
    }
    void routerErrorMessageAdd(const std::string &msg)
    {
        if ("NONE" == _routerErrorMessage)
        {
            _routerErrorMessage = "Repeat router found!!! Please check the following configuration>>";
        }
        _routerErrorMessage += "\n";
        _routerErrorMessage += msg;
    }
    int resultMessagePrint()
    {
        if ("NONE" != _syntaxErrorMessage)
        {
            LOG_ERROR("%s", _syntaxErrorMessage.c_str());
            return ERROR;
        }
        if ("NONE" != _routerErrorMessage)
        {
            LOG_ERROR("%s", _routerErrorMessage.c_str());
            return ERROR;
        }

        LOG_INFO("Config Check Pass");

        return SUCCESS;
    }

    // Get CurrentType's key string
    std::vector<std::string> GetConfigByType(const std::string &typeName) const
    {
        std::vector<std::string> ConfigHandleList;
        auto ret = ruleEngine.getRuleListByTypeStr(typeName);
        for (auto it : ret)
        {
            ConfigHandleList.emplace_back(it);
        }

        return ConfigHandleList;
    }

    std::unordered_map<json, std::vector<std::string>> collectConfigKeyPairs()
    {
        std::unordered_map<json, std::vector<std::string>> keyValuePairs;
        for (auto &jsonData : _configList)
        {
            LOG_DEBUG("[%s]\n%s", __func__, jsonData.dump(4).c_str());
            keyValuePairs[jsonData] = GetConfigByType(jsonData["type"]);
        }
        return keyValuePairs;
    }

    void StartCheck()
    {
        if (_configList.empty())
        {
            LOG_INFO("[%s] empty config !!!", __func__);
            return;
        }
        bool checkflag = true;
        // 语法检查
        for (auto &jsonData : _configList)
        {
            if (false == ruleEngine.applyRule(jsonData))
            {
                syntaxErrorMessageAdd(jsonData.dump(4));
                checkflag = false;
            }
        }

        if (true == checkflag)
        {
            // 路由重复性检查
            std::unordered_set<std::string> uniqueNames;

            for (const auto &jsonObj : _configList)
            {
                if (jsonObj.contains("router") && jsonObj["router"].is_string())
                {
                    std::string nameValue = jsonObj["router"];
                    if (!uniqueNames.insert(nameValue).second)
                    {
                        routerErrorMessageAdd(jsonObj.dump(4));
                    }
                }
            }
        }
    }
};

class AppConfigManager
{

private:
    /* obj */
    GrammarCheck _grammarCheck;
    /* data */
    std::string _configPath;
    /* const */
    const std::string _ConfigDir_c{"RouterConfigs"};

public:
    AppConfigManager(/* args */)
    {
        // get config Path
        _configPath = beiklive::TOOL::GetPwd() + "/../" + _ConfigDir_c;
    }
    ~AppConfigManager() = default;

    /// 检查目录下所有配置文件
    bool directoryCheck()
    {
        auto fileList = beiklive::TOOL::listFilesInDirectory(_configPath);
        for (const auto &fileName : fileList)
        {
            if (FAILED == parseFile(_configPath + "/" + fileName))
            {
                return false;
            }
        }
        return true;
    }
    /// 检查配置目录，获取所有配置文件
    int parseFile(const std::string &fileName)
    {
        try
        {
            std::ifstream f(fileName);
            json data = json::parse(f);
            for (const auto &[key, value] : data.items())
            {
                _grammarCheck.configAdd(value);
            }
            return SUCCESS;
        }
        catch (const json::parse_error &e)
        {
            LOG_ERROR("\nError parsing JSON file : %s\n%s", fileName.c_str(), e.what());
            return FAILED;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("\nException:\n%s", e.what());
            return FAILED;
        }
    }
    /// 解析配置的内容
    void parseConfig(const json &data)
    {
        for (const auto &[key, value] : data.items())
        {
            std::cout << "Key: " << key << ", Value: " << value << std::endl;
        }
    }

    int grammarAnalysis()
    {
        _grammarCheck.StartCheck();
        int ret = _grammarCheck.resultMessagePrint();
        return ret;
    }

    std::unordered_map<json, std::vector<t_string>> getHandlePairs()
    {
        return _grammarCheck.collectConfigKeyPairs();
    }

    void configClear(){
        _grammarCheck.configClean();
    }
};

class AuthManager
{
private:
    int _Server_Port = 8899;
    std::string _Server_Address = "127.0.0.1";
    std::string _ServerResPath;
    const std::string configName = "./config/ManagerConfig.ini";

private:
    beiklive::CONFIG::Config *ManagerConfig;
    httplib::Server ManagerHttpServer;
    AppConfigManager configManager;

public:
    void ShowRequestInfo(const httplib::Request &request, const httplib::Response &resp)
    {
        // 获取请求方的信息
        std::string clientIP = request.remote_addr;
        int clientPort = request.remote_port;
        std::string method = request.method;
        std::string uri = request.target;
        std::string httpVersion = request.version;
        LOG_DEBUG("%s:%d\t|\t%s %s", clientIP.c_str(), clientPort, method.c_str(), uri.c_str());
    }

    void ShowResponseInfo(const httplib::Request &request, const httplib::Response &resp)
    {
        // 获取请求方的信息
        std::string clientIP = request.remote_addr;
        int clientPort = request.remote_port;
        std::string method = request.method;
        std::string uri = request.target;
        std::string httpVersion = request.version;
        LOG_INFO("%d | %s:%d\t| %s %s %s", resp.status, clientIP.c_str(), clientPort, httpVersion.c_str(), method.c_str(), uri.c_str());
    }

public:
    AuthManager()
    {
        ManagerConfig = new beiklive::CONFIG::Config(configName);
        _ServerResPath = beiklive::TOOL::GetPwd() + "/../asserts";

        // 创建 log 和 config 目录
        if (beiklive::TOOL::createDirectoryIfNotExists("log") && beiklive::TOOL::createDirectoryIfNotExists("config"))
        {
            LOG_DEBUG("Directory exists or created successfully.");
        }
        else
        {
            LOG_ERROR("Failed to create directory.");
        }
    }

#undef SET_CONFIG_STR
#define SET_CONFIG_STR(cate, type, data)        \
    ManagerConfig->setConfig(cate, type, data); \
    LOG_INFO("Init %s\t>> %s\t: %s", cate, type, data.c_str());

#undef GET_CONFIG_STR
#define GET_CONFIG_STR(cate, type, data)                                      \
    std::string readData##data = ManagerConfig->getConfig(cate, type);        \
    if (readData##data != "")                                                 \
    {                                                                         \
        data = readData##data;                                                \
        LOG_INFO("Read %s\t>> %s\t: %s", cate, type, readData##data.c_str()); \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        LOG_DEBUG("Config has been broken, now goto init");                   \
        goto InitConfig;                                                      \
    }

#undef GET_CONFIG_INT
#define GET_CONFIG_INT(cate, type, data)                                      \
    std::string readData##data = ManagerConfig->getConfig(cate, type);        \
    if (readData##data != "")                                                 \
    {                                                                         \
        data = std::stoi(readData##data);                                     \
        LOG_INFO("Read %s\t>> %s\t: %s", cate, type, readData##data.c_str()); \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        LOG_DEBUG("Config has been broken, now goto init");                   \
        goto InitConfig;                                                      \
    }

    // 配置初始化  读取/新建
    void ConfigInit()
    {
        if (!ManagerConfig->loadFromFile())
        {
        InitConfig:
            LOG_INFO("Init ManagerConfig");
            // set
            SET_CONFIG_STR("NetWork", "Port", std::to_string(_Server_Port))
            SET_CONFIG_STR("NetWork", "Address", _Server_Address)
            SET_CONFIG_STR("Resource", "ResourcePath", _ServerResPath)
        }
        else
        {
            LOG_INFO("Read ManagerConfig");
            // read
            GET_CONFIG_INT("NetWork", "Port", _Server_Port)
            GET_CONFIG_STR("NetWork", "Address", _Server_Address)
            GET_CONFIG_STR("Resource", "ResourcePath", _ServerResPath)
        }
    }

#undef GET_CONFIG_INT
#undef SET_CONFIG_STR


void addTextHandle(const json &jsonObject)
{
    std::string router = jsonObject["router"];
    std::string txt = jsonObject["text"];
    LOG_INFO("Handle Set: text | %s\t\t|  %s", router.c_str(), txt.c_str());
    ManagerHttpServer.Get(router,
        [=](const httplib::Request& req, httplib::Response &res) {
            res.set_content(txt, "text/html");
        });
}

void addHtmlHandle(const json &jsonObject)
{
    std::string router = jsonObject["router"];
    std::string path = jsonObject["path"];
    LOG_INFO("Handle Set: html | %s\t\t|  %s", router.c_str(), path.c_str());
    auto ret = ManagerHttpServer.set_mount_point(router, path);
    if (!ret) {
    // The specified base directory doesn't exist...
    }
}

void addProgramHandle(const json &jsonObject)
{
    std::string router = jsonObject["router"];
    std::string cmd = jsonObject["command"];
    LOG_INFO("Handle Set: cmd | %s\t\t|  %s", router.c_str(), cmd.c_str());

    ManagerHttpServer.Get(router,
        [=](const httplib::Request& req, httplib::Response &res) {
            res.set_content("Executing : "+ cmd, "text/html");
            auto ret = beiklive::TOOL::executeCommand(cmd);
            res.set_content(ret, "text/html");
        });
}

    // 处理http请求
    void httpHandle()
    {
        // 静态文件根目录
        ManagerHttpServer.set_base_dir(_ServerResPath);

        // request 处理后调用
        ManagerHttpServer.set_logger([&](const httplib::Request &req, const httplib::Response &resp)
                                     { ShowResponseInfo(req, resp); });
        // 收到请求后的预处理
        ManagerHttpServer.set_pre_routing_handler([&](const auto &req, auto &resp)
                                                  { 
                                                    ShowRequestInfo(req, resp);
                                                    return httplib::Server::HandlerResponse::Unhandled; });

        ManagerHttpServer.set_error_handler([](const auto &req, auto &res)
                                            {
            auto fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
            char buf[BUFSIZ];
            snprintf(buf, sizeof(buf), fmt, res.status);
            res.set_content(buf, "text/html"); });

        ManagerHttpServer.Get("/bash",
        [=](const httplib::Request& req, httplib::Response &res){
            if (req.has_param("cmd")) {
                auto cmd = req.get_param_value("cmd");
                auto ret = beiklive::TOOL::executeCommand(cmd);
                res.set_content(ret, "text/plain;charset=utf-8");
            }else{
                res.set_content("Usage: /bash?cmd=xxxx", "text/plain;charset=utf-8");
            }
        }

        );



        //  expand config handle >>>>>>>>>>>>>
        auto jsonMap = configManager.getHandlePairs();
        for (const auto &pair : jsonMap)
        {
            const json &jsonObject = pair.first;
            const std::vector<std::string> &keys = pair.second;

            if(jsonObject["type"].is_string())
            {
                if("text" == jsonObject["type"])
                {
                    addTextHandle(jsonObject);
                }else if("html" == jsonObject["type"])
                {
                    addHtmlHandle(jsonObject);
                }else if("program" == jsonObject["type"])
                {
                    addProgramHandle(jsonObject);
                }
            }
        }
        //  end of config handle <<<<<<<<<<<<<
    }

    void ServerStart()
    {
        LOG_INFO("Start Listen >>  %s:%d", _Server_Address.c_str(), _Server_Port);
        ManagerHttpServer.listen(_Server_Address, _Server_Port);
    }





    void run()
    {
        LOG_INFO("Init >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
        ConfigInit();
        configManager.directoryCheck();
        if (SUCCESS == configManager.grammarAnalysis())
        {
            httpHandle();
            LOG_INFO("Done <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
            ServerStart();
        }
        else
        {
            LOG_ERROR("FAIL <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
        }
    }
};

int main(int argc, char **argv)
{
    beiklive::Logger::setLogLevel(beiklive::Logger::CommandLine::parseLogLevel(argc, argv));
    beiklive::Logger::initializeLogger(true);

    AuthManager ServerManger;
    ServerManger.run();

    return 0;
}
