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
	/*
	//因为仅仅执行work.reset并不能让iocontext从run的状态中退出
	//当iocontext已经绑定了读或写的监听事件后，还需要手动stop该服务。
	for (auto& work : _works) {
		//把服务先停止
		work->get_io_context().stop();
		work.reset();
	}

	for (auto& t : _threads) {
		t.join();
	}
	*/
	for (auto& work : workers_)
	{
		work.get_executor().context().stop();
		work.reset();
	}

	for (auto& t : threads_)
	{
		if (t.joinable()) {
			//std::cout << "thread is joinable." << std::endl;
			t.join();
		}else{
			//std::cout << "thread can't join." << std::endl;
		}
	}
}


