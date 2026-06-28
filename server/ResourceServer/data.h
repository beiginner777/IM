#ifndef DATA_H
#define DATA_H

#include <string>

struct UserInfo {
	UserInfo() :name_(""), pwd_(""), uid_(), email_(""), nick_(""), desc_(""), sex_(0), icon_(""), back_("")
	{
	}
	UserInfo(int uid, std::string name, std::string email,
		std::string pwd, std::string desc, std::string icon,
		int sex, std::string nick)
		:uid_(uid), name_(name), email_(email),
		pwd_(pwd), desc_(desc), icon_(icon),
		sex_(sex), nick_(nick)
	{
	}

	std::string name_;
	std::string pwd_;
	int uid_;
	std::string email_;
	std::string nick_;
	std::string desc_;
	int sex_;
	std::string icon_;
	std::string back_;
};


#endif
