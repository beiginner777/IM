#ifndef LOGINCSYSTEM_H
#define LOGINCSYSTEM_H

#include "global.h"
#include "SingleTon.h"
#include "HttpConnection.h"

class LogicSystem : public SingleTon<LogicSystem>
{
    friend class SingleTon<LogicSystem>;

    using Task = std::function<void()>;

public:
    LogicSystem();
    ~LogicSystem();

    void handleGetRequest(std::shared_ptr<HttpConnection> conn);
    void handlePostRequest(std::shared_ptr<HttpConnection> conn);

    //void postTaskToQue(Task task);

private:
    void registerGetHandler();
    void registerPostHandler();
    
    //void work();

    using getRequestHandler = std::function<void(std::shared_ptr<HttpConnection>)>;
	using postRequestHandler = std::function<void(std::shared_ptr<HttpConnection>)>;

	std::map<std::string, getRequestHandler> getHandles_;
	std::map<std::string, postRequestHandler> postHandles_;

    // 进行URL编码
    std::string urlEncode(std::string url);
    // 进行URL解码
    std::string urlDecode(std::string url);

    // 将一个十进制数字 转化为 十六进制下的字符形式
    unsigned char toHex(unsigned char ch);
    // 将一个十六进制字符形式 转化为 十进制下的数字形式
    unsigned char fromHex(unsigned char ch);

    // get请求的参数
    std::map<std::string,std::string> getPrama_;
    // post请求的参数
    std::map<std::string,std::string> postPrama_;

    std::queue<Task> que_;
    std::mutex mtx_;
    std::condition_variable cond_;
    
    //std::thread work_thread_;

    void prase_get_request(std::string& url);

    std::string url_;

    //std::atomic_bool b_stop_;
};

#endif