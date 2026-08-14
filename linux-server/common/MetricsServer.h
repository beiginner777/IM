#ifndef METRICS_SERVER_H
#define METRICS_SERVER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <iostream>
#include "MetricsRegistry.h"

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;
using tcp = boost::asio::ip::tcp;

// 极简 HTTP server：只响应 GET /metrics，返回 Prometheus 文本格式
// 给 TCP 服务（ChatServer/ResourceServer/StatusServer）暴露监控指标
//
// 用法：
//   auto metrics = std::make_shared<MetricsServer>(ioc, 9100);
//   metrics->start();
class MetricsServer : public std::enable_shared_from_this<MetricsServer>
{
public:
    MetricsServer(net::io_context& ioc, unsigned short port)
        : acceptor_(ioc, tcp::endpoint(tcp::v4(), port))
    {
        std::cout << "[MetricsServer] listening on :" << port << std::endl;
    }

    void start()
    {
        accept();
    }

private:
    void accept()
    {
        auto self = shared_from_this();
        acceptor_.async_accept([self](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                // 处理单次请求，处理完关闭连接
                auto conn = std::make_shared<Session>(std::move(socket));
                conn->run();
            }
            self->accept();  // 继续 accept 下一个
        });
    }

    // 单个连接处理：读请求 → 返回 /metrics → 关闭
    class Session : public std::enable_shared_from_this<Session>
    {
    public:
        explicit Session(tcp::socket socket) : socket_(std::move(socket)) {}

        void run()
        {
            readRequest();
        }

    private:
        void readRequest()
        {
            auto self = shared_from_this();
            http::async_read(socket_, buffer_, request_,
                [self](beast::error_code ec, std::size_t) {
                    if (!ec) self->handleRequest();
                    // 出错或处理完 → socket 析构自动关闭
                });
        }

        void handleRequest()
        {
            http::response<http::string_body> res;
            res.version(request_.version());
            res.keep_alive(false);

            if (request_.method() == http::verb::get &&
                request_.target() == "/metrics") {
                res.result(http::status::ok);
                res.set(http::field::content_type, "text/plain; version=0.0.4");
                res.body() = MetricsRegistry::getInstance()->render();
            } else {
                res.result(http::status::not_found);
                res.set(http::field::content_type, "text/plain");
                res.body() = "404 not found\n";
            }
            res.prepare_payload();

            auto self = shared_from_this();
            http::async_write(socket_, res,
                [self](beast::error_code ec, std::size_t) {
                    // 写完关闭连接
                    beast::error_code ec2;
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec2);
                });
        }

        tcp::socket socket_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> request_;
    };

    tcp::acceptor acceptor_;
};

#endif
