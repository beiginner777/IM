#ifndef SINGLETON_H
#define SINGLETON_H
#include <memory>
#include <mutex>
#include <iostream>
template<typename T>
class SingleTon
{
protected:
	SingleTon(const SingleTon<T>&) = delete;
	SingleTon& operator = (const SingleTon<T>&) = delete;
	SingleTon() = default;
	// 析构函数设置为protected可以使shared_ptr<Single<T>>对象正常析构
	~SingleTon() = default;
	static std::shared_ptr<T> instance_;
public:
	static std::shared_ptr<T> getInstance()
	{
		static std::once_flag s_flag;
		std::call_once(s_flag,[&]() {
		 	instance_ = std::shared_ptr<T>(new T);
		});
		return instance_;
	}
};
template<typename T>
std::shared_ptr<T> SingleTon<T>::instance_ = nullptr;
#endif