#include "ConfigManager.h"

ConfigManager::ConfigManager()
{
	// 获取当前工作目录
	boost::filesystem::path currentPath = boost::filesystem::current_path();

	// 构建config.ini的绝对路径
	boost::filesystem::path configPath = currentPath / "config.ini";

	//std::cout << "Config.init Path = " << configPath.string() << std::endl;
	
	// 读取ini文件到boost::property_tree::ptree结构当中
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(configPath.string(), pt);

	// 遍历boost::property_tree::ptree
	for (const auto& sectionPair : pt)
	{
		auto name = sectionPair.first;
		auto tree = sectionPair.second;
		std::map<std::string, std::string> sectionData;
		for (const auto& keyValue : tree)
		{
			const std::string key = keyValue.first;
			const std::string value = keyValue.second.get_value<std::string>();
			sectionData[key] = value;
		}
		SectionInfo sec;
		sec.setData(sectionData);
		configData_[name] = sec;
	}
	// 输出config.ini文件
	/*for (const auto& elem1 : configData_)
	{
		const auto& name = elem1.first;
		const auto& sec = elem1.second;
		std::cout << "[" << name << "]" << std::endl;
		auto secMap = sec.sectionData_;
		for (auto elem2 : secMap)
		{
			std::cout << elem2.first << " = " << elem2.second << std::endl;
		}
	}*/
}
