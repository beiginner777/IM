#ifndef WEBHOOKNOTIFIER_H
#define WEBHOOKNOTIFIER_H

#include <string>
#include <memory>
#include <sstream>
#include "AlertManager.h"

/// 企业微信 Webhook 通知（Mock 实现）
///
/// 当前为 Mock 模式：将告警消息转发到 AlertManager 日志输出
/// 未来升级：HTTP POST 到真实企业微信 Webhook URL
///
/// 用法：
///   WebhookNotifier::notify("WARNING", "Redis connection pool empty", "ChatServer1");
///
class WebhookNotifier
{
public:
	/// 发送告警通知
	/// @param level    严重级别（INFO / WARNING / CRITICAL）
	/// @param message  告警内容
	/// @param source   告警来源（Server 名称）
	static void notify(const std::string& level,
	                   const std::string& message,
	                   const std::string& source = "")
	{
		// Mock 实现：格式化后输出到 AlertManager 日志
		// 未来替换为 HTTP POST: https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=xxx
		std::ostringstream oss;
		oss << "[Webhook][" << level << "]";
		if (!source.empty()) oss << " [" << source << "]";
		oss << " " << message;
		AlertManager::getInstance()->warn(oss.str());
	}
};

#endif
