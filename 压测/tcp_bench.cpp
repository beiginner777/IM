// tcp_bench.cpp —— 单聊消息吞吐压测客户端（ChatServer 长连接 TCP）
// 编译: g++ -O2 -pthread tcp_bench.cpp -o tcp_bench -lboost_system -lssl -lcrypto
// 用法: ./tcp_bench <host> <port> <connections> <duration_sec> <uid_base> <jwt_secret> [target_uid]
// 例:   ./tcp_bench 127.0.0.1 8090 100 60 1000 im-jwt-secret-2026

#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <cstring>
#include <mutex>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <arpa/inet.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using boost::asio::ip::tcp;
using namespace std::chrono;

// ===== 协议常量（与 ChatServer1/global.h 一致）=====
static const int    HEAD_UUID_LEN            = 36;
static const int    HEAD_TOTOL_LEN_WITH_UUID = 40;  // 36B uuid + 2B msg_id + 2B len
static const short  ID_CHAT_LOGIN            = 1004;
static const short  ID_CHAT_LOGIN_RSP        = 1005;
static const short  ID_TEXT_CHAT_MSG_REQ     = 1014;
static const short  ID_TEXT_CHAT_MSG_RSP     = 1015;

// ===== 全局统计 =====
std::atomic<long> g_acks{0};
std::atomic<long> g_sent{0};   // 发送总数（含重发），丢失 = g_sent - g_acks
std::mutex g_mtx;
std::vector<long> g_latUs;   // 每条消息 ACK 延迟(微秒)

// ===== base64url + hex =====
static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static std::string b64url(const std::string& raw) {
    std::string out;
    for (size_t i = 0; i < raw.size(); i += 3) {
        unsigned v = (unsigned char)raw[i] << 16;
        if (i + 1 < raw.size()) v |= (unsigned char)raw[i + 1] << 8;
        if (i + 2 < raw.size()) v |= (unsigned char)raw[i + 2];
        out += B64[(v >> 18) & 0x3F];
        out += B64[(v >> 12) & 0x3F];
        out += (i + 1 < raw.size()) ? B64[(v >> 6) & 0x3F] : '=';
        out += (i + 2 < raw.size()) ? B64[v & 0x3F] : '=';
    }
    while (!out.empty() && out.back() == '=') out.pop_back();
    return out;
}
static std::string hex(const unsigned char* d, size_t n) {
    static const char* H = "0123456789abcdef";
    std::string s; s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { s += H[d[i] >> 4]; s += H[d[i] & 0xF]; }
    return s;
}

// 生成与 AuthServer 一致的 JWT（HMAC-SHA256，签名为 hex）
static std::string makeJwt(int uid, const std::string& secret) {
    long now = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    std::string header  = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    std::string payload = "{\"uid\":" + std::to_string(uid)
        + ",\"username\":\"bench" + std::to_string(uid) + "\""
        + ",\"client_type\":\"desktop\""
        + ",\"iat\":" + std::to_string(now)
        + ",\"exp\":" + std::to_string(now + 86400) + "}";
    std::string toSign = b64url(header) + "." + b64url(payload);
    unsigned char mac[32]; unsigned int macLen = 32;
    HMAC(EVP_sha256(), secret.data(), (int)secret.size(),
         (const unsigned char*)toSign.data(), toSign.size(), mac, &macLen);
    return toSign + "." + hex(mac, macLen);
}

// 36 字符 uuid（压测用，全局唯一即可）
// 原实现只取 LCG 低 4 位(n&15)当字符，周期 16，导致大量重复 uuid，
// 触发服务端 MessageDeduplicator 按 uuid 串话，返回错误用户的数据。
static std::string genUuid() {
    thread_local boost::uuids::random_generator gen;
    return boost::uuids::to_string(gen());
}

// 组帧：36B uuid + 2B msg_id(大端) + 2B len(大端) + body
static std::vector<char> makeFrame(short msgId, const std::string& body, const std::string& uuid) {
    short idNet  = htons(msgId);
    short lenNet = htons((short)body.size());
    std::vector<char> f(HEAD_TOTOL_LEN_WITH_UUID + body.size());
    memcpy(f.data(), uuid.data(), HEAD_UUID_LEN);
    memcpy(f.data() + 36, &idNet, 2);
    memcpy(f.data() + 38, &lenNet, 2);
    memcpy(f.data() + 40, body.data(), body.size());
    return f;
}

static bool readN(tcp::socket& s, void* buf, size_t n) {
    size_t off = 0;
    while (off < n) {
        boost::system::error_code ec;
        size_t got = s.read_some(boost::asio::buffer((char*)buf + off, n - off), ec);
        if (ec) return false;
        off += got;
    }
    return true;
}
static bool readFrame(tcp::socket& s, short& msgId, std::string& body) {
    char head[HEAD_TOTOL_LEN_WITH_UUID];
    if (!readN(s, head, sizeof(head))) return false;
    short idNet, lenNet;
    memcpy(&idNet,  head + 36, 2);
    memcpy(&lenNet, head + 38, 2);
    msgId = ntohs(idNet);
    short len = ntohs(lenNet);
    if (len < 0 || len > 4096) return false;
    body.resize(len);
    if (len && !readN(s, &body[0], len)) return false;
    return true;
}

void worker(const std::string& host, const std::string& port,
            int uid, const std::string& token, int targetUid, int durationSec, int dupCount) {
    try {
        boost::asio::io_context ioc;
        tcp::socket sock(ioc);
        tcp::resolver r(ioc);
        boost::asio::connect(sock, r.resolve(host, port));

        // 1) 登录
        std::string lb = "{\"uid\":" + std::to_string(uid) + ",\"token\":\"" + token + "\"}";
        auto f = makeFrame(ID_CHAT_LOGIN, lb, genUuid());
        boost::asio::write(sock, boost::asio::buffer(f.data(), f.size()));
        short id; std::string body;
        if (!readFrame(sock, id, body)) {
            std::cerr << "uid " << uid << " login failed: connection closed by server\n"; return;
        }
        if (id != ID_CHAT_LOGIN_RSP) {
            std::cerr << "uid " << uid << " login failed: unexpected msgId=" << id
                      << ", response=" << body << "\n"; return;
        }
        // msgId==1005 也可能是业务失败（用户不存在/限流等），打印服务端返回的具体原因
        if (body.find("\"code\" : 0") == std::string::npos) {
            std::cerr << "uid " << uid << " login rejected: " << body << "\n"; return;
        }

        // 2) 循环发消息 → 等 ACK（dupCount>1 时同 unique_id 重发，测服务端幂等去重）
        auto end = steady_clock::now() + seconds(durationSec);
        while (steady_clock::now() < end) {
            auto t0 = steady_clock::now();
            std::string uniq = genUuid();   // 同一条消息的 unique_id（重发复用）
            bool ok = true;
            for (int d = 0; d < dupCount; ++d) {
                std::string mb = "{\"fromuid\":" + std::to_string(uid)
                    + ",\"touid\":" + std::to_string(targetUid)
                    + ",\"thread_id\":1"
                    + ",\"text_array\":[{\"content\":\"hello\",\"unique_id\":\"" + uniq + "\"}]}";
                f = makeFrame(ID_TEXT_CHAT_MSG_REQ, mb, genUuid());
                boost::asio::write(sock, boost::asio::buffer(f.data(), f.size()));
                g_sent++;
                if (!readFrame(sock, id, body) || id != ID_TEXT_CHAT_MSG_RSP) { ok = false; break; }
                g_acks++;
            }
            if (!ok) break;
            long lat = duration_cast<microseconds>(steady_clock::now() - t0).count();
            { std::lock_guard<std::mutex> lk(g_mtx); g_latUs.push_back(lat); }
        }
    } catch (const std::exception& e) {
        std::cerr << "worker uid " << uid << " error: " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 7) {
        std::cerr << "usage: tcp_bench <host> <port> <conns> <duration> <uid_base> <jwt_secret> [target_uid] [dup_count]\n";
        return 1;
    }
    std::string host = argv[1], port = argv[2];
    int conns = atoi(argv[3]), duration = atoi(argv[4]);
    int uidBase = atoi(argv[5]);
    std::string secret = argv[6];
    int targetUid = (argc >= 8) ? atoi(argv[7]) : uidBase + 1;
    int dupCount = (argc >= 9) ? atoi(argv[8]) : 1;   // 每条消息重发次数，测 0 重复时传 2

    auto t0 = steady_clock::now();
    std::vector<std::thread> threads;
    for (int i = 0; i < conns; ++i) {
        int uid = uidBase + i;
        std::string token = makeJwt(uid, secret);
        threads.emplace_back(worker, host, port, uid, token, targetUid, duration, dupCount);
    }
    for (auto& t : threads) t.join();
    long elapsed = duration_cast<seconds>(steady_clock::now() - t0).count();
    if (elapsed == 0) elapsed = 1;

    long acks = g_acks.load();
    std::sort(g_latUs.begin(), g_latUs.end());
    auto pct = [&](double p) {
        if (g_latUs.empty()) return 0.0;
        return g_latUs[(size_t)(g_latUs.size() * p)] / 1000.0;
    };
    double avg = acks ? (double)std::accumulate(g_latUs.begin(), g_latUs.end(), 0L) / acks / 1000.0 : 0.0;

    std::cout << "=== Results ===\n"
              << "Connections : " << conns << "\n"
              << "Duration    : " << elapsed << "s\n"
              << "Sent        : " << g_sent.load() << "\n"
              << "ACKs        : " << acks << "\n"
              << "Lost        : " << (g_sent.load() - acks) << "\n"
              << "QPS         : " << (acks / elapsed) << "\n"
              << "Avg latency : " << avg << "ms\n"
              << "P50         : " << pct(0.50) << "ms\n"
              << "P99         : " << pct(0.99) << "ms\n";
    return 0;
}
