#include <memory>
#include <mutex>
#include <iostream>

template <typename T>
class SingleTon {
protected:
    SingleTon() = default;
    SingleTon(const SingleTon<T>&) = delete;
    SingleTon& operator=(const SingleTon<T>& st) = delete;

    static std::shared_ptr<T> instance_;
public:
    static std::shared_ptr<T> GetInstance() {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            instance_ = std::shared_ptr<T>(new T);
        });

        return instance_;
    }
    void getAddress() {
        std::cout << instance_.get() << std::endl;
    }
    virtual ~SingleTon() {
        //std::cout << "this is singleton destruct" << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> SingleTon<T>::instance_ = nullptr;
