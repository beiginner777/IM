#ifndef UTILS_H
#define UTILS_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <sstream>
#include <string>

// 获取当前时间的字符串表示，格式为 "YYYY-MM-DD HH:MM:SS"
std::string GetCurrentTimestamp();

#endif