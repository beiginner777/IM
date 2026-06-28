#include "AsioIOContextThreadPool.h"

AsioIOContextThreadPool::AsioIOContextThreadPool(size_t size)
	: io_contexts_(size)
	, iocontextIndex(0)
{
	std::cout << size << " cores Server starts " << std::endl;

	for (int i = 0; i < size; ++i)
	{
		auto worker = boost::asio::make_work_guard(io_contexts_[i]);
		workers_.emplace_back(std::move(worker));
	}
	for (int i = 0; i < size; ++i)
	{
		threads_.emplace_back([this,i]() {
			io_contexts_[i].run();
		});
	}

}

AsioIOContextThreadPool::~AsioIOContextThreadPool()
{
	stop();
	std::cout << "AsioIOContextThreadPool stop. " << std::endl;
}

boost::asio::io_context& AsioIOContextThreadPool::getIOContext()
{
	boost::asio::io_context& ioc = io_contexts_[iocontextIndex++];

	//std::cout << iocontextIndex << std::endl;

	if (iocontextIndex == io_contexts_.size()) {
		iocontextIndex = 0;
	}
	return ioc;
}

void AsioIOContextThreadPool::stop()
{
	for (auto& work : workers_)
	{
		boost::asio::io_context& io_ref = static_cast<boost::asio::io_context&>(work.get_executor().context());
		io_ref.stop();
	}

	for (int i = 0; i < threads_.size(); ++i)
	{
		threads_[i].join();
	}
}


