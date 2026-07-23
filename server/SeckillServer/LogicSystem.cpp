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

LogicSystem::LogicSystem() : mysqlDao_(new MysqlDao()) {
	registerGetHandler();
}

LogicSystem::~LogicSystem() { delete mysqlDao_; }

void LogicSystem::registerGetHandler() {
	getHandles_["/products"] = [this](std::shared_ptr<HttpConnection> conn) {
		Json::Value arr(Json::arrayValue);
		for (const auto& p : mysqlDao_->getProducts()) {
			Json::Value item; item["id"]=p.id; item["name"]=p.name; item["price"]=p.price;
			item["stock"]=p.stock; item["imageUrl"]=p.imageUrl; arr.append(item);
		}
		sendJson(conn, arr);
	};
	getHandles_["/rank"] = [this](std::shared_ptr<HttpConnection> conn) {
		Json::Value arr(Json::arrayValue);
		auto counts = mysqlDao_->getBuyCounts();
		auto products = mysqlDao_->getProducts();
		for (const auto& kv : counts) {
			Json::Value item; item["productId"]=kv.first; item["count"]=kv.second;
			for (auto& p : products) if(p.id==kv.first) item["productName"]=p.name;
			arr.append(item);
		}
		sendJson(conn, arr);
	};
	getHandles_["/orders"] = [this](std::shared_ptr<HttpConnection> conn) {
		Json::Value arr(Json::arrayValue);
		for (auto& o : mysqlDao_->getOrders()) {
			Json::Value item; item["orderId"]=o.id; item["productName"]=o.productName;
			item["time"]=o.time; item["status"]="成功"; arr.append(item);
		}
		sendJson(conn, arr);
	};
	getHandles_["/profile"] = [this](std::shared_ptr<HttpConnection> conn) {
		Json::Value v;
		if (!conn->authenticate()) { v["error"]="请先登录"; sendJson(conn,v); return; }
		v["uid"] = conn->uid();
		v["username"] = mysqlDao_->getUsername(conn->uid());
		v["balance"] = mysqlDao_->getBalance(conn->uid());
		Json::Value orders(Json::arrayValue);
		for (auto& o : mysqlDao_->getOrdersByUid(conn->uid())) {
			Json::Value item; item["orderId"]=o.id; item["productName"]=o.productName;
			item["price"]=o.price; item["status"]=o.status; item["time"]=o.time; orders.append(item);
		}
		v["orders"] = orders;
		sendJson(conn, v);
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

void LogicSystem::handleBuy(std::shared_ptr<HttpConnection> conn, int productId, const std::string& bodyStr) {
	Json::Value v;
	if (!conn->authenticate()) { v["success"]=false; v["message"]="请先登录"; sendJson(conn,v); return; }
	// 查余额
	double balance = mysqlDao_->getBalance(conn->uid());
	if (balance < 0) { v["success"]=false; v["message"]="查询余额失败"; sendJson(conn,v); return; }
	// 查商品 (MySQL)
	auto products = mysqlDao_->getProducts();
	auto it = std::find_if(products.begin(), products.end(), [productId](const MysqlDao::Product& p){return p.id==productId;});
	if (it == products.end()) { v["success"]=false; v["message"]="商品不存在"; sendJson(conn,v); return; }
	if (it->stock <= 0) { v["success"]=false; v["message"]="已售罄"; sendJson(conn,v); return; }
	if (balance < it->price) { v["success"]=false; v["message"]="余额不足"; sendJson(conn,v); return; }
	int orderId = mysqlDao_->insertOrder(conn->uid(), productId, it->name, it->price);
	if (orderId < 0) { v["success"]=false; v["message"]="创建订单失败"; sendJson(conn,v); return; }
	v["success"]=true; v["orderId"]=orderId; v["message"]="订单已创建";
	sendJson(conn, v);
	std::cout<<"[SeckillServer] uid="<<conn->uid()<<" order:"<<orderId<<" "<<it->name<<std::endl;
}

void LogicSystem::handleRecharge(std::shared_ptr<HttpConnection> conn, const std::string& bodyStr) {
	Json::Value v;
	if (!conn->authenticate()) { v["code"]=-1; v["message"]="请先登录"; sendJson(conn,v); return; }
	Json::Value req; Json::Reader reader; reader.parse(bodyStr, req);
	std::string pwd = req["password"].asString();
	double amount = req["amount"].asDouble();
	if (amount <= 0) { 
		v["code"] = -1; 
		v["message"] = "金额无效"; 
		sendJson(conn,v); 
		return; 
	}
	if (pwd.empty() || !mysqlDao_->verifyPassword(conn->uid(), pwd)) {
		v["code"] = -1; 
		v["message"] = "密码错误"; 
		sendJson(conn,v); 
		return;
	}
	double cur = mysqlDao_->getBalance(conn->uid());
	if (cur < 0) { 
		v["code"] = -1; 
		v["message"] = "查询余额失败"; 
		sendJson(conn,v); 
		return; 
	}
	double newBal = cur + amount;
	if (!mysqlDao_->updateBalance(conn->uid(), newBal)) {
		v["code"] = -1; 
		v["message"] = "充值失败";
		sendJson(conn,v);
		return;
	}
	RedisManager::getInstance()->Del("balance_cache:"+std::to_string(conn->uid()));
	v["code"]=0; 
	v["balance"] = newBal;
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
	if (getHandles_.count(url_)) { getHandles_[url_](conn); url_=""; getPrama_.clear(); return; }
	// GET /order/{id}
	static const std::string kOrderGet = "/order/";
	if (url_.find(kOrderGet) == 0 && url_.find("/pay")==std::string::npos && url_.find("/cancel")==std::string::npos) {
		int oid = atoi(url_.substr(kOrderGet.size()).c_str());
		Json::Value v;
		if (!conn->authenticate()) { v["error"]="请先登录"; sendJson(conn,v); }
		else {
			auto o = mysqlDao_->getOrderById(oid);
			if (o.id<0) { v["error"]="not found"; sendJson(conn,v); }
			else {
				}
				v["id"]=o.id; v["uid"]=o.uid; v["productId"]=o.productId; v["productName"]=o.productName;
				v["price"]=o.price; v["status"]=o.status; v["time"]=o.time;
				sendJson(conn, v);
			}
		}
		url_=""; getPrama_.clear(); return;
	}
	conn->response_.result(http::status::not_found); Json::Value v; v["error"]="not found"; sendJson(conn,v);
	url_=""; getPrama_.clear();
}

void LogicSystem::handlePostRequest(std::shared_ptr<HttpConnection> conn) {
	std::string target = conn->request_.target();
	std::string bodyStr = beast::buffers_to_string(conn->request_.body().data());
	std::cout << "[SeckillServer] POST " << target << std::endl;
	for (auto& h : conn->request_) {
		std::cout << "  " << h.name_string() << ": " << h.value() << std::endl;
	}
	std::cout << "  body: " << bodyStr << std::endl;
	if (target == "/recharge") { handleRecharge(conn, bodyStr); return; }
	static const std::string kBuyPrefix = "/buy/";
	if (target.find(kBuyPrefix) == 0) { handleBuy(conn, atoi(target.substr(kBuyPrefix.size()).c_str()), bodyStr); return; }
	static const std::string kPayPrefix = "/order/";
	if (target.find(kPayPrefix) == 0) {
		std::string rest = target.substr(kPayPrefix.size());
		if (rest.find("/pay") != std::string::npos) {
			int oid = atoi(rest.substr(0, rest.find("/pay")).c_str());
			handlePayOrder(conn, oid, bodyStr);
		} else if (rest.find("/cancel") != std::string::npos) {
			int oid = atoi(rest.substr(0, rest.find("/cancel")).c_str());
			handleCancelOrder(conn, oid);
		}
		return;
	}
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

void LogicSystem::handlePayOrder(std::shared_ptr<HttpConnection> conn, int orderId, const std::string& bodyStr) {
	Json::Value v;
	if (!conn->authenticate()) { v["success"]=false; v["message"]="请先登录"; sendJson(conn,v); return; }
	Json::Value req; Json::Reader reader; reader.parse(bodyStr, req);
	if (!mysqlDao_->verifyPassword(conn->uid(), req["password"].asString())) {
		v["success"]=false; v["message"]="密码错误"; sendJson(conn,v); return;
	}
	auto order = mysqlDao_->getOrderById(orderId);
	if (order.id<0 || order.uid!=conn->uid()) { v["success"]=false; v["message"]="订单不存在"; sendJson(conn,v); return; }
	if (order.status!="unpaid") { v["success"]=false; v["message"]="订单状态异常"; sendJson(conn,v); return; }
	double balance = mysqlDao_->getBalance(conn->uid());
	if (balance < order.price) { v["success"]=false; v["message"]="余额不足"; sendJson(conn,v); return; }
	double newBal = balance - order.price;
	if (!mysqlDao_->updateBalance(conn->uid(), newBal)) { v["success"]=false; v["message"]="扣款失败"; sendJson(conn,v); return; }
	auto products = mysqlDao_->getProducts();
	for (auto& p : products) {
		if (p.id==order.productId) { mysqlDao_->updateStock(p.id, p.stock-1); break; }
	}
	mysqlDao_->payOrder(orderId, conn->uid());
	RedisManager::getInstance()->Del("balance_cache:"+std::to_string(conn->uid()));
	v["success"]=true; v["message"]="支付成功"; v["balance"]=newBal;
	sendJson(conn, v);
}

void LogicSystem::handleCancelOrder(std::shared_ptr<HttpConnection> conn, int orderId) {
	Json::Value v;
	if (!conn->authenticate()) { v["success"]=false; v["message"]="请先登录"; sendJson(conn,v); return; }
	auto order = mysqlDao_->getOrderById(orderId);
	if (order.id<0 || order.uid!=conn->uid()) { v["success"]=false; v["message"]="订单不存在"; sendJson(conn,v); return; }
	mysqlDao_->cancelOrder(orderId, conn->uid());
	v["success"]=true; v["message"]="订单已取消，30分钟后自动删除";
	sendJson(conn, v);
}
