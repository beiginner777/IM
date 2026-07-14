#ifndef ASIOIOCONTEXTTHREAD_H
#define ASIOIOCONTEXTTHREAD_H

#include <boost/asio.hpp>
#include <vector>
#include <thread>
#include "SingleTon.h"
class AsioIOContextThreadPool : public SingleTon<AsioIOContextThreadPool>
{
	friend class SingleTon<AsioIOContextThreadPool>;
public:
	AsioIOContextThreadPool(size_t size = std::thread::hardware_concurrency());
	~AsioIOContextThreadPool();
	boost::asio::io_context& getIOContext();
	void stop();
private:
	using WorkPtr = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
	std::vector<boost::asio::io_context> io_contexts_;
	std::vector<WorkPtr> workers_;
	std::vector<std::thread> threads_;
	int iocontextIndex;
};
#endif
