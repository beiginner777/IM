#include "LogicSystem.h"
#include "MysqlDao.h"
#include "JWT.h"
#include "crypto/BCryptHasher.h"
#include "RedisManager.h"

static std::string nowString() {
	std::time_t t = std::time(nullptr); std::tm tm; char buf[32]{};
	localtime_s(&tm, &t); std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm); return buf;
}

void LogicSystem::sendJson(std::shared_ptr<HttpConnection> conn, const Json::Value& value) {
	conn->response_.set(http::field::content_type, "application/json");
	beast::ostream(conn->response_.body()) << value.toStyledString();
}

LogicSystem::LogicSystem() : nextOrderId_(1), mysqlDao_(new MysqlDao()) {
	registerGetHandler();
}

LogicSystem::~LogicSystem() { delete mysqlDao_; }

void LogicSystem::registerGetHandler() {
	getHandles_["/products"] = [this](std::shared_ptr<HttpConnection> conn) {
		Json::Value arr(Json::arrayValue);
		for (const auto& p : products_) {
			Json::Value item; item["id"]=p.id; item["name"]=p.name; item["price"]=p.price;
			item["stock"]=p.stock; item["imageUrl"]=p.imageUrl; arr.append(item);
		}
		sendJson(conn, arr);
	};
	getHandles_["/rank"] = [this](std::shared_ptr<HttpConnection> conn) {
		Json::Value arr(Json::arrayValue);
		for (const auto& kv : buyCount_) {
			Json::Value item; item["productId"]=kv.first; item["count"]=kv.second;
			for (auto& p : products_) if(p.id==kv.first) item["productName"]=p.name;
			arr.append(item);
		}
		sendJson(conn, arr);
	};
	getHandles_["/orders"] = [this](std::shared_ptr<HttpConnection> conn) {
		Json::Value arr(Json::arrayValue);
		for (auto it=orders_.rbegin(); it!=orders_.rend(); ++it) {
			Json::Value item; item["orderId"]=it->orderId; item["productName"]=it->productName;
			item["time"]=it->time; item["status"]=it->status; arr.append(item);
		}
		sendJson(conn, arr);
	};
	getHandles_["/balance"] = [this](std::shared_ptr<HttpConnection> conn) {
		Json::Value v;
		if (!conn->authenticate()) { v["error"]="请先登录"; sendJson(conn,v); return; }
		double b = mysqlDao_->getBalance(conn->uid());
		if (b < 0) b = 0;
		v["balance"] = b;
		sendJson(conn, v);
	};
}

void LogicSystem::handleBuy(std::shared_ptr<HttpConnection> conn, int productId) {
	Json::Value v;
	if (!conn->authenticate()) { v["success"]=false; v["message"]="请先登录"; sendJson(conn,v); return; }
	// 获取密码
	std::string bodyStr = beast::buffers_to_string(conn->request_.body().data());
	Json::Value req; Json::Reader reader; reader.parse(bodyStr, req);
	std::string pwd = req["password"].asString();
	if (pwd.empty() || !mysqlDao_->verifyPassword(conn->uid(), pwd)) {
		v["success"]=false; v["message"]="密码错误"; sendJson(conn,v); return;
	}
	// 查余额
	double balance = mysqlDao_->getBalance(conn->uid());
	if (balance < 0) { v["success"]=false; v["message"]="查询余额失败"; sendJson(conn,v); return; }
	// 查商品
	auto it = std::find_if(products_.begin(), products_.end(), [productId](const SeckillProduct& p){return p.id==productId;});
	if (it == products_.end()) { v["success"]=false; v["message"]="商品不存在"; sendJson(conn,v); return; }
	if (it->stock <= 0) { v["success"]=false; v["message"]="已售罄"; sendJson(conn,v); return; }
	if (balance < it->price) { v["success"]=false; v["message"]="余额不足"; sendJson(conn,v); return; }
	// 扣余额
	double newBal = balance - it->price;
	if (!mysqlDao_->updateBalance(conn->uid(), newBal)) {
		v["success"]=false; v["message"]="扣款失败"; sendJson(conn,v); return;
	}
	// 扣库存
	it->stock--;
	buyCount_[productId]++;
	orders_.push_back({nextOrderId_++, it->name, nowString(), "成功"});
	// 清缓存
	RedisManager::getInstance()->Del("balance_cache:"+std::to_string(conn->uid()));
	v["success"] = true;
	v["message"] = "购买成功";
	v["balance"] = newBal;
	sendJson(conn, v);
	std::cout<<"[SeckillServer] uid="<<conn->uid()<<" bought "<<it->name<<" balance="<<newBal<<" stock="<<it->stock<<std::endl;
}

void LogicSystem::handleRecharge(std::shared_ptr<HttpConnection> conn) {
	Json::Value v;
	if (!conn->authenticate()) { v["code"]=-1; v["message"]="请先登录"; sendJson(conn,v); return; }
	std::string bodyStr = beast::buffers_to_string(conn->request_.body().data());
	Json::Value req; Json::Reader reader; reader.parse(bodyStr, req);
	std::string pwd = req["password"].asString();
	double amount = req["amount"].asDouble();
	if (amount <= 0) { v["code"]=-1; v["message"]="金额无效"; sendJson(conn,v); return; }
	if (pwd.empty() || !mysqlDao_->verifyPassword(conn->uid(), pwd)) {
		v["code"]=-1; v["message"]="密码错误"; sendJson(conn,v); return;
	}
	double cur = mysqlDao_->getBalance(conn->uid());
	if (cur < 0) { v["code"]=-1; v["message"]="查询余额失败"; sendJson(conn,v); return; }
	double newBal = cur + amount;
	if (!mysqlDao_->updateBalance(conn->uid(), newBal)) {
		v["code"]=-1; v["message"]="充值失败"; sendJson(conn,v); return;
	}
	RedisManager::getInstance()->Del("balance_cache:"+std::to_string(conn->uid()));
	v["code"]=0; v["balance"]=newBal;
	sendJson(conn, v);
}

void LogicSystem::handleGetBalance(std::shared_ptr<HttpConnection> conn) {
	Json::Value v;
	if (!conn->authenticate()) { v["error"]="请先登录"; sendJson(conn,v); return; }
	double b = mysqlDao_->getBalance(conn->uid());
	v["balance"] = b < 0 ? 0 : b;
	sendJson(conn, v);
}

void LogicSystem::sendAuthError(std::shared_ptr<HttpConnection> conn, const std::string& msg) {
	Json::Value v; v["success"]=false; v["message"]=msg; sendJson(conn, v);
}

void LogicSystem::handleGetRequest(std::shared_ptr<HttpConnection> conn) {
	std::string target = conn->request_.target();
	prase_get_request(target);
	if (getHandles_.count(url_)) { getHandles_[url_](conn); }
	else { conn->response_.result(http::status::not_found); Json::Value v; v["error"]="not found"; sendJson(conn,v); }
	url_=""; getPrama_.clear();
}

void LogicSystem::handlePostRequest(std::shared_ptr<HttpConnection> conn) {
	std::string target = conn->request_.target();
	if (target == "/recharge") { handleRecharge(conn); return; }
	static const std::string kBuyPrefix = "/buy/";
	if (target.find(kBuyPrefix) == 0) { handleBuy(conn, atoi(target.substr(kBuyPrefix.size()).c_str())); return; }
	conn->response_.result(http::status::not_found); Json::Value v; v["error"]="not found"; sendJson(conn,v);
}

unsigned char LogicSystem::toHex(unsigned char ch) { if(ch>9) return ch+55; return ch+48; }
unsigned char LogicSystem::fromHex(unsigned char ch) {
	if(ch>='A'&&ch<='Z') return ch-'A'+10; else if(ch>='a'&&ch<='z') return ch-'a'+10;
	else if(ch>='0'&&ch<='9') return ch-'0'; else return '0';
}
std::string LogicSystem::urlEncode(std::string url) {
	std::string r;
	for(int i=0;i<url.size();++i) {
		if(url[i]==' ') r+='+';
		else if(url[i]=='-'||url[i]=='_'||url[i]=='.'||url[i]=='~'||isalnum(url[i])) r+=url[i];
		else { r+="%"; r+=toHex((unsigned char)url[i]>>4); r+=toHex((unsigned char)url[i]&0x0F); }
	}
	return r;
}
std::string LogicSystem::urlDecode(std::string url) {
	std::string r;
	for(int i=0;i<url.size();++i) {
		if(url[i]=='+') r+=' ';
		else if(url[i]=='%') { unsigned char h=fromHex((unsigned char)url[++i]); unsigned char l=fromHex((unsigned char)url[++i]); r+=(char)((h<<4)+l); }
		else r+=url[i];
	}
	return r;
}
void LogicSystem::prase_get_request(std::string& url) {
	auto q=url.find('?'); if(q==std::string::npos){url_=url; return;}
	url_=url.substr(0,q); std::string qs=url.substr(q+1);
	size_t p=0; std::string k,val;
	while((p=qs.find('&'))!=std::string::npos) {
		auto pair=qs.substr(0,p); size_t e=pair.find('=');
		if(e!=std::string::npos){ k=urlDecode(pair.substr(0,e)); val=urlDecode(pair.substr(e+1)); getPrama_[k]=val; }
		qs.erase(0,p+1);
	}
	if(!qs.empty()){ size_t e=qs.find('='); if(e!=std::string::npos){ k=urlDecode(qs.substr(0,e)); val=urlDecode(qs.substr(e+1)); getPrama_[k]=val; } }
}
