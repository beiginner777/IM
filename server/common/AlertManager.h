#ifndef ALERT_MANAGER_H
#define ALERT_MANAGER_H

#include "SingleTon.h"
#include <memory>
#include <string>
namespace spdlog { class logger; }
// 统一日志/告警入口（封装 spdlog 异步日志）
// 用法: AlertManager::getInstance()->info("xxx");
//       AlertManager::getInstance()->warn("xxx");
//       AlertManager::getInstance()->crit("xxx");
//
// 日志输出:
//   - 控制台（dev 环境直接看）
//   - logs/im_server.log（滚动文件，5MB x 3）
//
// 后续 AlertServer 接入时，CRITICAL 级别通过此入口推送给远程服务。
class AlertManager : public SingleTon<AlertManager>
{
	friend class SingleTon<AlertManager>;
public:
	AlertManager();
	~AlertManager();
	void info(const std::string& msg);
	void warn(const std::string& msg);
	void crit(const std::string& msg);
private:
	std::shared_ptr<spdlog::logger> logger_;
};
#endif
