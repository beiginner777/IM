#include "global.h"

std::string generate_unique_string()
{
    // 创建uuid对象
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    // 将uuid对象转化为字符串
    std::string unique_string = to_string(uuid);

    return unique_string;
}
