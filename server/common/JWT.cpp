#include "JWT.h"
#include "ConfigManager.h"

const std::string& JWT::secret()
{
	static std::string s;
	if (s.empty()) {
		auto cfg = ConfigManager::getInstance();
		s = cfg["JWT"]["Secret"];
		if (s.empty()) s = "im-secret-key-2026";  // 默认值
	}
	return s;
}
