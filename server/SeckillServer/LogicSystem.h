#ifndef LOGINCSYSTEM_H
#define LOGINCSYSTEM_H

#include "global.h"
#include "SingleTon.h"
#include "HttpConnection.h"
#include "MysqlDao.h"
// 秒杀商品（Mock 阶段存内存，后续任务替换为 Redis 库存 + MySQL 订单 + RabbitMQ）
struct SeckillProduct
{
	int id;
	std::string name;
	int price;
	int stock;
	std::string imageUrl;
};
// 抢购记录（Mock 阶段存内存）
struct OrderRecord
{
	int orderId;
	std::string productName;
	std::string time;
	std::string status;
};
class LogicSystem : public SingleTon<LogicSystem>
{
	friend class SingleTon<LogicSystem>;
public:
	LogicSystem();
	~LogicSystem();
	void handleGetRequest(std::shared_ptr<HttpConnection> conn);
	void handlePostRequest(std::shared_ptr<HttpConnection> conn);
private:
	void registerGetHandler();
	void handleBuy(std::shared_ptr<HttpConnection> conn, int productId);
	void handleRecharge(std::shared_ptr<HttpConnection> conn);
	void handleGetBalance(std::shared_ptr<HttpConnection> conn);
	void sendJson(std::shared_ptr<HttpConnection> conn, const Json::Value& value);
	void sendAuthError(std::shared_ptr<HttpConnection> conn, const std::string& msg);

	using getRequestHandler = std::function<void(std::shared_ptr<HttpConnection>)>;
	std::map<std::string, getRequestHandler> getHandles_;
	// 进行URL编码
	std::string urlEncode(std::string url);
	// 进行URL解码
	std::string urlDecode(std::string url);
	// 将一个十进制数字 转化为 十六进制下的字符形式
	unsigned char toHex(unsigned char ch);
	// 将一个十六进制字符形式 转化为 十进制下的数字形式
	unsigned char fromHex(unsigned char ch);
	void prase_get_request(std::string& url);
	// get请求的参数
	std::map<std::string, std::string> getPrama_;
	std::string url_;

	// ==================== Mock 数据（内存态） ====================
	std::vector<SeckillProduct> products_;
	std::map<int, int> buyCount_;
	std::vector<OrderRecord> orders_;
	int nextOrderId_;
	std::mutex dataMtx_;
	MysqlDao* mysqlDao_{nullptr};
};
#endif
