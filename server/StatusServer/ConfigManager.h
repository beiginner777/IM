#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "global.h"
#include "SingleTon.h"

class SectionInfo
{
public:
	SectionInfo() {}
	SectionInfo(const SectionInfo& sec)
	{
		this->sectionData_ = sec.sectionData_;
	}
	SectionInfo& operator = (const SectionInfo& sec)
	{
		if (&sec == this)
		{
			return *this;
		}
		this->sectionData_ = sec.sectionData_;
		return *this;
	}
	std::string operator[] (const std::string& key)
	{
		if (sectionData_.count(key)) {
			return sectionData_[key];
		}
		return "";
	}
	~SectionInfo() 
	{
		sectionData_.clear();
	}

	std::map<std::string, std::string> getdate() { return sectionData_; }
	void setData(std::map<std::string, std::string> data) { sectionData_ = data; }

	std::map<std::string, std::string> sectionData_;
};

// 因为需要使用这个类的实例化对象，而不是指针。
// 所以不使用SingleTon<T>,重新写一个静态获取实例化对象的函数
class ConfigManager
{
	friend class SingleTon<ConfigManager>;
public:
	static ConfigManager getInstance() {
		static ConfigManager cfg;
		return cfg;
	}
	ConfigManager();
	ConfigManager(const ConfigManager& cfg)
	{
		this->configData_ = cfg.configData_;
	}
	ConfigManager& operator = (const ConfigManager& cfg)
	{
		if (&cfg == this) 
		{
			return *this;
		}
		this->configData_ = cfg.configData_;
		return *this;
	}
	SectionInfo operator[](const std::string& key)
	{
		if (configData_.count(key))
		{
			return configData_[key];
		}
		return SectionInfo();
	}
	~ConfigManager() 
	{
		configData_.clear();
	}
private:
	std::map<std::string, SectionInfo> configData_;
};

#endif