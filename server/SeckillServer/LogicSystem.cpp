#include "LogicSystem.h"
// 当前时间字符串，格式: 2026-07-18 15:04:05
static std::string nowString()
{
	std::time_t t = std::time(nullptr);
	std::tm tmv;
	localtime_s(&tmv, &t);
	char buf[32] = { 0 };
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
	return buf;
}
// 统一 JSON 响应
void LogicSystem::sendJson(std::shared_ptr<HttpConnection> conn, const Json::Value& value)
{
	auto& response = conn->response_;
	response.set(http::field::content_type, "application/json");
	beast::ostream(response.body()) << value.toStyledString();
}
LogicSystem::LogicSystem()
	: nextOrderId_(1)
{
	// Mock 商品数据（后续任务替换为 MySQL 商品表 + Redis 预热库存）
	products_ = {
		{ 1, "iPhone 15 Pro 限时秒杀", 4999, 50,  "https://picsum.photos/seed/iphone/400/300" },
		{ 2, "RTX 4090 显卡",          9999, 20,  "https://picsum.photos/seed/gpu/400/300" },
		{ 3, "Switch OLED 游戏机",     1499, 100, "https://picsum.photos/seed/switch/400/300" },
		{ 4, "AirPods Pro 2",          999,  200, "https://picsum.photos/seed/airpods/400/300" },
	};
	registerGetHandler();
}
LogicSystem::~LogicSystem()
{
	std::cout << "LogicSystem destructed." << std::endl;
}

void LogicSystem::registerGetHandler()
{
	// GET /products —— 商品列表，前端期望裸数组 [{id,name,price,stock,imageUrl}]
	getHandles_["/products"] = [this](std::shared_ptr<HttpConnection> conn) {
		Json::Value arr(Json::arrayValue);
		{
			std::lock_guard<std::mutex> lock(dataMtx_);
			for (const auto& p : products_) {
				Json::Value item;
				item["id"] = p.id;
				item["name"] = p.name;
				item["price"] = p.price;
				item["stock"] = p.stock;
				item["imageUrl"] = p.imageUrl;
				arr.append(item);
			}
		}
		sendJson(conn, arr);
	};
	// GET /rank —— 排行榜，前端轮询，期望裸数组 [{productId,productName,count}]
	// 后续任务替换为 Redis ZSET（ZREVRANGE）
	getHandles_["/rank"] = [this](std::shared_ptr<HttpConnection> conn) {
		std::vector<std::pair<int, int>> rank; // {productId, count}
		{
			std::lock_guard<std::mutex> lock(dataMtx_);
			for (const auto& kv : buyCount_) {
				rank.push_back(kv);
			}
		}
		std::sort(rank.begin(), rank.end(),
			[](const std::pair<int, int>& a, const std::pair<int, int>& b) { return a.second > b.second; });
		Json::Value arr(Json::arrayValue);
		{
			std::lock_guard<std::mutex> lock(dataMtx_);
			for (const auto& kv : rank) {
				auto it = std::find_if(products_.begin(), products_.end(),
					[&kv](const SeckillProduct& p) { return p.id == kv.first; });
				if (it == products_.end()) continue;
				Json::Value item;
				item["productId"] = kv.first;
				item["productName"] = it->name;
				item["count"] = kv.second;
				arr.append(item);
			}
		}
		sendJson(conn, arr);
	};
	// GET /orders?username=xxx —— 抢购记录，前端期望裸数组 [{orderId,productName,time,status}]
	// 注意：前端 POST /buy 未携带用户身份，Mock 阶段返回全部记录，不按 username 过滤；
	// 待接入统一认证（JWT）后再关联到用户
	getHandles_["/orders"] = [this](std::shared_ptr<HttpConnection> conn) {
		Json::Value arr(Json::arrayValue);
		{
			std::lock_guard<std::mutex> lock(dataMtx_);
			for (auto it = orders_.rbegin(); it != orders_.rend(); ++it) {
				Json::Value item;
				item["orderId"] = it->orderId;
				item["productName"] = it->productName;
				item["time"] = it->time;
				item["status"] = it->status;
				arr.append(item);
			}
		}
		sendJson(conn, arr);
	};
}
// POST /buy/{productId}，前端期望 {success: bool, message: string}
void LogicSystem::handleBuy(std::shared_ptr<HttpConnection> conn, int productId)
{
	Json::Value value;
	std::lock_guard<std::mutex> lock(dataMtx_);
	auto it = std::find_if(products_.begin(), products_.end(),
		[productId](const SeckillProduct& p) { return p.id == productId; });
	if (it == products_.end()) {
		value["success"] = false;
		value["message"] = "商品不存在";
		sendJson(conn, value);
		return;
	}
	if (it->stock <= 0) {
		value["success"] = false;
		value["message"] = "手慢了，已售罄";
		sendJson(conn, value);
		return;
	}
	// Mock 扣库存（内存态，后续任务替换为 Redis Lua 原子扣减 + RabbitMQ 异步落单）
	it->stock--;
	buyCount_[productId]++;
	orders_.push_back({ nextOrderId_++, it->name, nowString(), "成功" });
	value["success"] = true;
	value["message"] = "抢购成功";
	sendJson(conn, value);
	std::cout << "[SeckillServer] product(" << it->name << ") sold, stock left = " << it->stock << std::endl;
}
void LogicSystem::handleGetRequest(std::shared_ptr<HttpConnection> conn)
{
	std::string target = conn->request_.target();
	// 解码出来 URL放在 url_变量当中; 然后将 键值对 放在getPrama_中
	prase_get_request(target);
	std::cout << "prase url = " << url_ << std::endl;
	if (getHandles_.count(url_))
	{
		getHandles_[url_](conn);
	}
	else
	{
		auto& response = conn->response_;
		response.result(http::status::not_found);
		response.set(http::field::content_type, "application/json");
		Json::Value value;
		value["error_code"] = ERROE_CODR::ERROR_PARAM;
		value["error_msg"] = "url not found";
		beast::ostream(response.body()) << value.toStyledString();
	}
	url_ = "";
	getPrama_.clear();
}
void LogicSystem::handlePostRequest(std::shared_ptr<HttpConnection> conn)
{
	std::string target = conn->request_.target();
	std::cout << "prase url = " << target << std::endl;
	// POST /buy/{productId}：路径参数，前缀匹配
	static const std::string kBuyPrefix = "/buy/";
	if (target.find(kBuyPrefix) == 0)
	{
		int productId = atoi(target.substr(kBuyPrefix.size()).c_str());
		handleBuy(conn, productId);
		return;
	}
	std::cout << "Can' not find PostHandler of " << target << std::endl;
	auto& response = conn->response_;
	response.result(http::status::not_found);
	response.set(http::field::content_type, "application/json");
	Json::Value value;
	value["error_code"] = ERROE_CODR::ERROR_PARAM;
	value["error_msg"] = "url not found";
	beast::ostream(response.body()) << value.toStyledString();
}
unsigned char LogicSystem::toHex(unsigned char ch)
{
	if (ch > 9) return ch + 55;
	else return ch + 48;
}

unsigned char LogicSystem::fromHex(unsigned char ch)
{
	if (ch >= 'A' && ch <= 'Z') return ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'z') return ch - 'a' + 10;
	else if (ch >= '0' && ch <= '9') return ch - '0';
	else assert(0);
	return '0';
}
// 将十进制转化为十六进制(eg: 将中文转化为十六进制字符)
std::string LogicSystem::urlEncode(std::string url)
{
	std::string result = "";
	for (int i = 0; i < url.size(); ++i)
	{
		if (url[i] == ' ') result += '+';
		else if (url[i] == '-' || url[i] == '_' || url[i] == '.' || url[i] == '~' || isalnum(url[i])) result += url[i];
		else {
			result += "%";
			result += toHex((unsigned char)url[i] >> 4);
			result += toHex((unsigned char)url[i] & 0x0F);
		}
	}
	return result;
}
// 将十六进制转化为十进制(eg: 将十六进制字符转化为中文)
std::string LogicSystem::urlDecode(std::string url)
{
	std::string result = "";
	for (int i = 0; i < url.size(); ++i)
	{
		if (url[i] == '+') result += ' ';
		//遇到%将后面的两个字符从16进制转为char再拼接
		else if (url[i] == '%')
		{
			unsigned char high = fromHex((unsigned char)url[++i]);
			unsigned char low = fromHex((unsigned char)url[++i]);
			result += (char)((high << 4) + low);
		}
		else result += url[i];
	}
	return result;
}

void LogicSystem::prase_get_request(std::string& url)
{
	// 查找查询字符串的开始位置（即 '?' 的位置）
	auto query_pos = url.find('?');
	if (query_pos == std::string::npos) {
		url_ = url;
		return;
	}
	url_ = url.substr(0, query_pos);
	std::string query_string = url.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = urlDecode(pair.substr(0, eq_pos));
			value = urlDecode(pair.substr(eq_pos + 1));
			getPrama_[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// 处理最后一个参数对（如果没有 & 分隔符）
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = urlDecode(query_string.substr(0, eq_pos));
			value = urlDecode(query_string.substr(eq_pos + 1));
			getPrama_[key] = value;
		}
	}
}
