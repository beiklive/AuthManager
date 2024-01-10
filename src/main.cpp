#include <iostream>
#include "../inc/Tools.hpp"
#include "../inc/cpp-httplib/httplib.h"

/*  验证管理器功能
1. 基本的 Get Post请求, 基础网页能力
2. Api 接口能力， 以配置文件驱动
*/

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

public:
    void ShowRequestIofo(const httplib::Request &request)
    {
        // 获取请求方的信息
        std::string clientIP = request.remote_addr;
        int clientPort = request.remote_port;
        std::string method = request.method;
        std::string uri = request.target;
        std::string httpVersion = request.version;
        LOG_INFO("Request Information: %s %s %s ----> %s:%d", httpVersion.c_str(), method.c_str(), uri.c_str(), clientIP.c_str(), clientPort);
    }

public:
    AuthManager()
    {
        ManagerConfig = new beiklive::CONFIG::Config(configName);
        _ServerResPath = beiklive::TOOL::GetPwd() + "/../res";

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

#define SET_CONFIG_STR(cate, type, data)        \
    ManagerConfig->setConfig(cate, type, data); \
    LOG_INFO("Init %s\t>> %s\t: %s", cate, type, data.c_str());

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
        LOG_INFO("Init >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
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
        LOG_INFO("Done <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
    }
    // 处理http请求
    void httpHandle()
    {
        // 静态文件根目录
        ManagerHttpServer.set_base_dir(_ServerResPath);

        ManagerHttpServer.Get("/", [&](const httplib::Request &request, httplib::Response &res)
                              { ShowRequestIofo(request); });

        ManagerHttpServer.Get("/hi", [&](const httplib::Request &request, httplib::Response &res)
                              { 
                                ShowRequestIofo(request);
                                res.set_content("Hello World!", "text/plain"); });
        ManagerHttpServer.Get("/stop",
                              [&](const httplib::Request & /*req*/, httplib::Response & /*res*/)
                              { ManagerHttpServer.stop(); });
    }

    void ServerStart()
    {
        LOG_INFO("Start Listen >>  %s:%d", _Server_Address.c_str(), _Server_Port);
        ManagerHttpServer.listen(_Server_Address, _Server_Port);
    }
};

int main(int argc, char **argv)
{
    beiklive::Logger::setLogLevel(beiklive::Logger::CommandLine::parseLogLevel(argc, argv));
    beiklive::Logger::initializeLogger(true);

    AuthManager ServerManger;
    ServerManger.ConfigInit();
    ServerManger.httpHandle();
    ServerManger.ServerStart();

    return 0;
}
