#include "ConfigManager.h"
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

std::string ConfigManager::configPath_;

static void loadIni(const std::string& path,
                    std::map<std::string, SectionInfo>& data)
{
    std::cout << "[ConfigManager] loading config from: " << path << std::endl;
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(path, pt);
    for (const auto& sec : pt) {
        std::map<std::string, std::string> kv;
        for (const auto& it : sec.second)
            kv[it.first] = it.second.get_value<std::string>();
        SectionInfo si; si.setData(kv);
        data[sec.first] = si;
    }
}

ConfigManager::ConfigManager()
{
    // 1. 命令行传了路径 → 直接用
    if (!configPath_.empty()) {
        loadIni(configPath_, configData_);
        return;
    }

    // 2. 按优先级搜索 "config.ini"
    std::vector<boost::filesystem::path> dirs;
    dirs.push_back(boost::filesystem::current_path());

    std::string exePath;
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    exePath = buf;
#else
    char buf[256];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = 0;
        exePath = buf;
    }
#endif
    boost::filesystem::path exe(exePath);

    if (!exePath.empty()) {
        dirs.push_back(exe.parent_path());
        dirs.push_back(exe.parent_path().parent_path());
    }

    for (auto& d : dirs) {
        auto p = d / "config.ini";
        if (boost::filesystem::exists(p)) {
            loadIni(p.string(), configData_);
            return;
        }
    }
    std::cerr << "[ConfigManager] config.ini not found" << std::endl;
}
