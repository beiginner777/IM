#include "AlertManager.h"
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <iostream>
#include <vector>
AlertManager::AlertManager()
{
	try {
		// 控制台 sink（带颜色）
		auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		consoleSink->set_level(spdlog::level::info);
		// 文件 sink（滚动：5MB x 3 个文件）
		auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
			"logs/im_server.log", 5 * 1024 * 1024, 3);
		fileSink->set_level(spdlog::level::info);
		std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };
		// 异步 logger：队列 8192 条，1 个后台线程。队列满时丢弃旧消息，不阻塞
		spdlog::init_thread_pool(8192, 1);
		logger_ = std::make_shared<spdlog::async_logger>(
			"im", sinks.begin(), sinks.end(),
			spdlog::thread_pool(),
			spdlog::async_overflow_policy::overrun_oldest);
		logger_->set_level(spdlog::level::info);
		logger_->flush_on(spdlog::level::warn);  // WARNING 及以上自动刷盘
		spdlog::register_logger(logger_);
		logger_->info("AlertManager started (async, console+file, rotate 5MBx3)");
	}
	catch (const std::exception& e) {
		std::cerr << "AlertManager init failed: " << e.what() << std::endl;
	}
}
AlertManager::~AlertManager()
{
	if (logger_) {
		logger_->info("AlertManager shutting down");
		logger_->flush();
	}
	spdlog::shutdown();
}
void AlertManager::info(const std::string& msg)
{
	if (logger_) logger_->info(msg);
}
void AlertManager::warn(const std::string& msg)
{
	if (logger_) logger_->warn(msg);
}
void AlertManager::crit(const std::string& msg)
{
	if (logger_) logger_->critical(msg);
}
