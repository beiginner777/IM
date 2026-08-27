#ifndef SSLUTIL_H
#define SSLUTIL_H
// SSL 工具：统一读取 PEM 证书 + 构造 gRPC TLS 凭据（全链路加密用）
// 证书由 docker/gen_certs.sh 生成，路径配置在各服务 config.ini [SSL] 段
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <grpcpp/grpcpp.h>

namespace sslutil {

// 读取 PEM 文件内容（证书/私钥）。失败返回空字符串。
inline std::string readPemFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "[SslUtil] 无法打开证书文件: " << path
                  << "（请先运行 docker/gen_certs.sh）" << std::endl;
        return "";
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

// gRPC 服务端凭据：加载服务器证书 + 私钥（客户端单向 TLS）
inline std::shared_ptr<grpc::ServerCredentials> makeServerCredentials(
    const std::string& certPath, const std::string& keyPath)
{
    grpc::SslServerCredentialsOptions opts;
    grpc::SslServerCredentialsOptions::PemKeyCertPair pair;
    pair.private_key = readPemFile(keyPath);
    pair.cert_chain  = readPemFile(certPath);
    if (pair.private_key.empty() || pair.cert_chain.empty()) {
        return grpc::InsecureServerCredentials();  // 降级：证书缺失则回退明文（便于排查）
    }
    opts.pem_key_cert_pairs.push_back(std::move(pair));
    return grpc::SslServerCredentials(opts);
}

// gRPC 客户端凭据：加载 CA 根证书校验服务端
inline std::shared_ptr<grpc::ChannelCredentials> makeChannelCredentials(
    const std::string& caCertPath)
{
    grpc::SslCredentialsOptions opts;
    opts.pem_root_certs = readPemFile(caCertPath);
    if (opts.pem_root_certs.empty()) {
        return grpc::InsecureChannelCredentials();  // 降级：证书缺失则回退明文（便于排查）
    }
    return grpc::SslCredentials(opts);
}

} // namespace sslutil

#endif
