#include "ConfigManager.h"
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>

ConfigManager::ConfigManager()
{
	// ��ȡ��ǰ����Ŀ¼
	boost::filesystem::path currentPath = boost::filesystem::current_path();

	// ����config.ini�ľ���·��
	boost::filesystem::path configPath = currentPath / "config.ini";

	//std::cout << "Config.init Path = " << configPath.string() << std::endl;
	
	// ��ȡini�ļ���boost::property_tree::ptree�ṹ����
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(configPath.string(), pt);

	// ����boost::property_tree::ptree
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
	// ���config.ini�ļ�
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
