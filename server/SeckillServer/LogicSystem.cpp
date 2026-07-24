#include "LogicSystem.h"
#include "MysqlDao.h"
#include "JWT.h"
#include "crypto/BCryptHasher.h"
#include "RedisManager.h"

void LogicSystem::sendJson(std::shared_ptr<HttpConnection> conn, const Json::Value& v)
{
	conn->response_.set(http::field::content_type, "application/json");
	beast::ostream(conn->response_.body()) << v.toStyledString();
}
void LogicSystem::sendAuthError(std::shared_ptr<HttpConnection> conn, const std::string& msg)
{
	Json::Value v;
	v["success"] = false;
	v["message"] = msg;
	sendJson(conn, v);
}

LogicSystem::LogicSystem() : mysqlDao_(new MysqlDao())
{
	registerGetHandler();
	registerPostHandler();
}
LogicSystem::~LogicSystem()
{
	delete mysqlDao_;
}

// ==================== GET handlers ====================
void LogicSystem::registerGetHandler()
{
	getHandles_["/products"] = [this](auto conn, auto&)
	{
		Json::Value arr(Json::arrayValue);
		for (auto& p : mysqlDao_->getProducts())
		{
			Json::Value i;
			i["id"] = p.id;
			i["name"] = p.name;
			i["price"] = p.price;
			i["stock"] = p.stock;
			i["imageUrl"] = p.imageUrl;
			arr.append(i);
		}
		sendJson(conn, arr);
	};
	getHandles_["/rank"] = [this](auto conn, auto&)
	{
		Json::Value arr(Json::arrayValue);
		auto counts = mysqlDao_->getBuyCounts();
		auto products = mysqlDao_->getProducts();
		for (auto& kv : counts)
		{
			Json::Value i;
			i["productId"] = kv.first;
			i["count"] = kv.second;
			for (auto& p : products)
				if (p.id == kv.first)
					i["productName"] = p.name;
			arr.append(i);
		}
		sendJson(conn, arr);
	};
	getHandles_["/profile"] = [this](auto conn, auto&)
	{
		Json::Value v;
		if (!conn->authenticate())
		{
			v["error"] = "请先登录";
			sendJson(conn, v);
			return;
		}
		v["uid"] = conn->uid();
		v["username"] = mysqlDao_->getUsername(conn->uid());
		v["balance"] = mysqlDao_->getBalance(conn->uid());
		Json::Value ord(Json::arrayValue);
		for (auto& o : mysqlDao_->getOrdersByUid(conn->uid()))
		{
			Json::Value i;
			i["orderId"] = o.id;
			i["productName"] = o.productName;
			i["price"] = o.price;
			i["status"] = o.status;
			i["time"] = o.time;
			ord.append(i);
		}
		v["orders"] = ord;
		sendJson(conn, v);
	};
	getHandles_["/balance"] = [this](auto conn, auto&)
	{
		Json::Value v;
		if (!conn->authenticate())
		{
			v["error"] = "请先登录";
			sendJson(conn, v);
			return;
		}
		v["balance"] = mysqlDao_->getBalance(conn->uid());
		sendJson(conn, v);
	};
	getHandles_["/order/"] = [this](auto conn, auto& p)
	{
		Json::Value v;
		if (!conn->authenticate())
		{
			v["error"] = "请先登录";
			sendJson(conn, v);
			return;
		}
		auto o = mysqlDao_->getOrderById(p.pathId);
		if (o.id < 0)
		{
			v["error"] = "not found";
			sendJson(conn, v);
			return;
		}
		v["id"] = o.id;
		v["uid"] = o.uid;
		v["productId"] = o.productId;
		v["productName"] = o.productName;
		v["price"] = o.price;
		v["status"] = o.status;
		v["time"] = o.time;
		if (o.status == "unpaid" && o.time.size() >= 19)
		{
			struct tm t = {};
			sscanf_s(o.time.c_str(), "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min,
			         &t.tm_sec);
			t.tm_year -= 1900;
			t.tm_mon -= 1;
			int remain = 1800 - (int) (time(nullptr) - mktime(&t));
			v["remainSeconds"] = remain > 0 ? remain : 0;
		}
		sendJson(conn, v);
	};
}

// ==================== POST handlers ====================
void LogicSystem::registerPostHandler()
{
	postHandles_["/recharge"] = [this](auto conn, auto& p)
	{
		Json::Value v, req;
		Json::Reader r;
		r.parse(p.body, req);
		if (!conn->authenticate())
		{
			v["code"] = -1;
			v["message"] = "请先登录";
			sendJson(conn, v);
			return;
		}
		if (req["amount"].asDouble() <= 0)
		{
			v["code"] = -1;
			v["message"] = "金额无效";
			sendJson(conn, v);
			return;
		}
		if (!mysqlDao_->verifyPassword(conn->uid(), req["password"].asString()))
		{
			v["code"] = -1;
			v["message"] = "密码错误";
			sendJson(conn, v);
			return;
		}
		double cur = mysqlDao_->getBalance(conn->uid()), nb = cur + req["amount"].asDouble();
		if (!mysqlDao_->updateBalance(conn->uid(), nb))
		{
			v["code"] = -1;
			v["message"] = "充值失败";
			sendJson(conn, v);
			return;
		}
		RedisManager::getInstance()->Del("balance_cache:" + std::to_string(conn->uid()));
		v["code"] = 0;
		v["balance"] = nb;
		sendJson(conn, v);
	};
	postHandles_["/buy/"] = [this](auto conn, auto& p)
	{
		Json::Value v;
		if (!conn->authenticate())
		{
			v["success"] = false;
			v["message"] = "请先登录";
			sendJson(conn, v);
			return;
		}
		auto prods = mysqlDao_->getProducts();
		auto it = std::find_if(prods.begin(), prods.end(), [&](auto& x) { return x.id == p.pathId; });
		if (it == prods.end())
		{
			v["success"] = false;
			v["message"] = "商品不存在";
			sendJson(conn, v);
			return;
		}
		if (it->stock <= 0)
		{
			v["success"] = false;
			v["message"] = "已售罄";
			sendJson(conn, v);
			return;
		}
		if (mysqlDao_->getBalance(conn->uid()) < it->price)
		{
			v["success"] = false;
			v["message"] = "余额不足";
			sendJson(conn, v);
			return;
		}
		int oid = mysqlDao_->insertOrder(conn->uid(), p.pathId, it->name, it->price);
		v["success"] = true;
		v["orderId"] = oid;
		v["message"] = "订单已创建";
		sendJson(conn, v);
	};
	postHandles_["/order/"] = [this](auto conn, auto& p)
	{
		Json::Value v, req;
		Json::Reader r;
		r.parse(p.body, req);
		if (!conn->authenticate())
		{
			v["success"] = false;
			v["message"] = "请先登录";
			sendJson(conn, v);
			return;
		}
		if (p.url.find("/pay") != std::string::npos)
		{
			if (!mysqlDao_->verifyPassword(conn->uid(), req["password"].asString()))
			{
				v["success"] = false;
				v["message"] = "密码错误";
				sendJson(conn, v);
				return;
			}
			auto o = mysqlDao_->getOrderById(p.pathId);
			if (o.id < 0 || o.uid != conn->uid() || o.status != "unpaid")
			{
				v["success"] = false;
				v["message"] = "订单异常";
				sendJson(conn, v);
				return;
			}
			double bal = mysqlDao_->getBalance(conn->uid());
			if (bal < o.price)
			{
				v["success"] = false;
				v["message"] = "余额不足";
				sendJson(conn, v);
				return;
			}
			if (!mysqlDao_->updateBalance(conn->uid(), bal - o.price))
			{
				v["success"] = false;
				v["message"] = "扣款失败";
				sendJson(conn, v);
				return;
			}
			for (auto& pr : mysqlDao_->getProducts())
				if (pr.id == o.productId)
				{
					mysqlDao_->updateStock(pr.id, pr.stock - 1);
					break;
				}
			mysqlDao_->payOrder(p.pathId, conn->uid());
			RedisManager::getInstance()->Del("balance_cache:" + std::to_string(conn->uid()));
			v["success"] = true;
			v["message"] = "支付成功";
			sendJson(conn, v);
		}
		else if (p.url.find("/cancel") != std::string::npos)
		{
			mysqlDao_->cancelOrder(p.pathId, conn->uid());
			v["success"] = true;
			v["message"] = "订单已取消";
			sendJson(conn, v);
		}
		else
		{
			v["success"] = false;
			v["message"] = "unknown";
			sendJson(conn, v);
		}
	};
}

// ==================== URL 解析 + 前缀路由 ====================
void LogicSystem::handleGetRequest(std::shared_ptr<HttpConnection> conn)
{
	std::string target = conn->request_.target();
	GetParams p;
	prase_get_request(target);
	p.url = url_;
	p.query = getPrama_;
	url_ = "";
	getPrama_.clear();
	if (p.url.find("/order/") == 0)
		p.pathId = atoi(p.url.substr(7).c_str());
	for (auto& [prefix, handler] : getHandles_)
		if (p.url.find(prefix) == 0)
		{
			handler(conn, p);
			return;
		}
	conn->response_.result(http::status::not_found);
	Json::Value v;
	v["error"] = "not found";
	sendJson(conn, v);
}

void LogicSystem::handlePostRequest(std::shared_ptr<HttpConnection> conn)
{
	std::string target = conn->request_.target();
	PostParams p;
	p.body = beast::buffers_to_string(conn->request_.body().data());
	p.url = target;
	if (target.find("/order/") == 0)
		p.pathId = atoi(target.substr(7).c_str());
	else if (target.find("/buy/") == 0)
		p.pathId = atoi(target.substr(5).c_str());
	for (auto& [prefix, handler] : postHandles_)
		if (target.find(prefix) == 0)
		{
			handler(conn, p);
			return;
		}
	conn->response_.result(http::status::not_found);
	Json::Value v;
	v["error"] = "not found";
	sendJson(conn, v);
}

// ==================== URL 编解码 ====================
unsigned char LogicSystem::toHex(unsigned char ch)
{
	return ch > 9 ? ch + 55 : ch + 48;
}
unsigned char LogicSystem::fromHex(unsigned char ch)
{
	if (ch >= 'A' && ch <= 'Z')
		return ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'z')
		return ch - 'a' + 10;
	else if (ch >= '0' && ch <= '9')
		return ch - '0';
	return 0;
}
std::string LogicSystem::urlEncode(std::string url)
{
	std::string r;
	for (int i = 0; i < (int) url.size(); ++i)
	{
		if (url[i] == ' ')
			r += '+';
		else if (isalnum((unsigned char) url[i]) || url[i] == '-' || url[i] == '_' || url[i] == '.' || url[i] == '~')
			r += url[i];
		else
		{
			r += "%";
			r += toHex((unsigned char) url[i] >> 4);
			r += toHex((unsigned char) url[i] & 0x0F);
		}
	}
	return r;
}
std::string LogicSystem::urlDecode(std::string url)
{
	std::string r;
	for (int i = 0; i < (int) url.size(); ++i)
	{
		if (url[i] == '+')
			r += ' ';
		else if (url[i] == '%')
		{
			r += (char) ((fromHex((unsigned char) url[++i]) << 4) + fromHex((unsigned char) url[++i]));
		}
		else
			r += url[i];
	}
	return r;
}
void LogicSystem::prase_get_request(std::string& url)
{
	auto q = url.find('?');
	if (q == std::string::npos)
	{
		url_ = url;
		return;
	}
	url_ = url.substr(0, q);
	std::string qs = url.substr(q + 1);
	size_t p = 0;
	std::string k, val;
	while ((p = qs.find('&')) != std::string::npos)
	{
		auto pair = qs.substr(0, p);
		size_t e = pair.find('=');
		if (e != std::string::npos)
		{
			k = urlDecode(pair.substr(0, e));
			val = urlDecode(pair.substr(e + 1));
			getPrama_[k] = val;
		}
		qs.erase(0, p + 1);
	}
	if (!qs.empty())
	{
		size_t e = qs.find('=');
		if (e != std::string::npos)
		{
			k = urlDecode(qs.substr(0, e));
			val = urlDecode(qs.substr(e + 1));
			getPrama_[k] = val;
		}
	}
}
