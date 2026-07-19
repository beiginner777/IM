#ifndef JWT_H
#define JWT_H
#include <string>
#include <json/json.h>
#include <boost/beast/core/detail/base64.hpp>
#include <openssl/hmac.h>
#include <chrono>
#include <sstream>

class JWT
{
public:
	// 密钥从 config.ini 读取
	static const std::string& secret();

	// 生成 token: header.payload.signature
	static std::string generateToken(int uid, const std::string& username)
	{
		Json::Value header;
		header["alg"] = "HS256";
		header["typ"] = "JWT";
		std::string headerB64 = base64url(header.toStyledString());

		Json::Value payload;
		payload["uid"] = uid;
		payload["username"] = username;
		auto now = std::chrono::system_clock::now();
		auto exp = now + std::chrono::hours(24);
		payload["exp"] = std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count();
		std::string payloadB64 = base64url(payload.toStyledString());

		std::string toSign = headerB64 + "." + payloadB64;
		std::string sig = hmacSha256(toSign);
		return toSign + "." + sig;
	}

	// 验证 token，成功返回 true 并写入 uid
	static bool verify(const std::string& token, int& uid)
	{
		size_t dot1 = token.find('.');
		size_t dot2 = token.rfind('.');
		if (dot1 == std::string::npos || dot2 == std::string::npos || dot1 == dot2)
			return false;

		std::string toVerify = token.substr(0, dot2);
		std::string expectedSig = hmacSha256(toVerify);
		std::string actualSig = token.substr(dot2 + 1);
		if (expectedSig != actualSig)
			return false;

		std::string payloadB64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
		std::string payloadJson = base64urlDecode(payloadB64);
		Json::Value payload;
		Json::Reader reader;
		if (!reader.parse(payloadJson, payload))
			return false;

		long long exp = payload["exp"].asInt64();
		auto now = std::chrono::system_clock::now();
		auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
		if (nowSec > exp)
			return false;

		uid = payload["uid"].asInt();
		return true;
	}

private:
	// Base64URL 编码（去掉了填充符 =，替换 +/ 为 -_）
	static std::string base64url(const std::string& raw)
	{
		std::string enc;
		enc.resize(boost::beast::detail::base64::encoded_size(raw.size()));
		boost::beast::detail::base64::encode(enc.data(), raw.data(), raw.size());
		// 去除 '\r\n' 填充等
		enc.erase(std::remove(enc.begin(), enc.end(), '\r'), enc.end());
		enc.erase(std::remove(enc.begin(), enc.end(), '\n'), enc.end());
		// 替换为标准 Base64URL
		for (char& c : enc) {
			if (c == '+') c = '-';
			else if (c == '/') c = '_';
			else if (c == '=') { enc.resize(&c - enc.data()); break; }
		}
		return enc;
	}

	static std::string base64urlDecode(const std::string& enc)
	{
		std::string tmp = enc;
		for (char& c : tmp) {
			if (c == '-') c = '+';
			else if (c == '_') c = '/';
		}
		// 补回填充符
		while (tmp.size() % 4) tmp += '=';
		std::string raw;
		raw.resize(boost::beast::detail::base64::decoded_size(tmp.size()));
		auto result = boost::beast::detail::base64::decode(raw.data(), tmp.data(), tmp.size());
		raw.resize(result.first);
		return raw;
	}

	static std::string hmacSha256(const std::string& data)
	{
		unsigned char result[EVP_MAX_MD_SIZE];
		unsigned int len = 0;
		const std::string& key = secret();
		HMAC(EVP_sha256(), key.c_str(), static_cast<int>(key.size()),
		     reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), result, &len);
		return base64url(std::string(reinterpret_cast<char*>(result), len));
	}
};

#endif // JWT_H
