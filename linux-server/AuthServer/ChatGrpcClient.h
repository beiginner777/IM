#ifndef AUTH_CHATGRPCCLIENT_H
#define AUTH_CHATGRPCCLIENT_H
// AuthServer 专用的轻量 gRPC 客户端 —— 仅用于 NotifyKickUser
// 使用连接池模式，参考 ChatServer1/ChatGrpcClient.h
#include "global.h"
#include "message.grpc.pb.h"
#include "ConfigManager.h"
#include "SslUtil.h"
#include <unordered_map>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace message;

// gRPC 连接池（同 ChatServer1 ChatConnPool）
class KickConnPool
{
public:
    KickConnPool(std::size_t poolSize, std::string host, std::string port)
        : poolSize_(poolSize), host_(host), port_(port), b_stop_(false)
    {
        for (std::size_t i = 0; i < poolSize; ++i) {
            auto channel = grpc::CreateChannel(host + ":" + port,
                sslutil::makeChannelCredentials(ConfigManager::getInstance()["SSL"]["CaCert"]));
            connections_.push(ChatService::NewStub(channel));
        }
    }

    std::unique_ptr<ChatService::Stub> getConnection()
    {
        std::unique_lock<std::mutex> locker(mtx_);
        while (!b_stop_ && connections_.empty()) {
            if (std::cv_status::timeout == cond_.wait_for(locker, std::chrono::milliseconds(100))) {
                std::cout << "[KickConnPool] pool busy, retrying..." << std::endl;
                return nullptr;
            }
        }
        if (b_stop_) return nullptr;
        auto conn = std::move(connections_.front());
        connections_.pop();
        return conn;
    }

    void returnConnection(std::unique_ptr<ChatService::Stub> conn)
    {
        if (b_stop_) return;
        std::lock_guard<std::mutex> locker(mtx_);
        connections_.push(std::move(conn));
        cond_.notify_one();
    }

private:
    std::size_t poolSize_;
    std::string host_, port_;
    std::atomic_bool b_stop_;
    std::queue<std::unique_ptr<ChatService::Stub>> connections_;
    std::mutex mtx_;
    std::condition_variable cond_;
};

class KickUserClient : public SingleTon<KickUserClient>
{
    friend class SingleTon<KickUserClient>;
public:
    // 通知指定 ChatServer 踢掉 uid 的旧连接
    KickUserRsp NotifyKickUser(const std::string& serverName, int uid)
    {
        KickUserRsp rsp;
        auto it = pools_.find(serverName);
        if (it == pools_.end()) {
            auto cfg = ConfigManager::getInstance();
            std::string host = cfg[serverName]["Host"];
            std::string rpcPort = cfg[serverName]["RPCPort"];
            if (host.empty() || rpcPort.empty()) {
                std::cerr << "[KickUser] No config for " << serverName << std::endl;
                rsp.set_error(-1);
                return rsp;
            }
            pools_[serverName] = std::make_unique<KickConnPool>(5, host, rpcPort);
            it = pools_.find(serverName);
        }

        KickUserReq req;
        req.set_uid(uid);
        ClientContext context;
        auto stub = it->second->getConnection();
        if (!stub) {
            rsp.set_error(-1);
            return rsp;
        }
        auto status = stub->NotifyKickUser(&context, req, &rsp);
        it->second->returnConnection(std::move(stub));
        if (!status.ok()) {
            std::cerr << "[KickUser] gRPC failed: " << status.error_message() << std::endl;
            rsp.set_error(-1);
        }
        return rsp;
    }

private:
    KickUserClient() {}
    std::unordered_map<std::string, std::unique_ptr<KickConnPool>> pools_;
};

#endif
