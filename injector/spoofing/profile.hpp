#pragma once
#include <nlohmann/json.hpp>

namespace coinpoker {
namespace spoofing {
class Profile {
public:
	Profile() {};
	Profile(const nlohmann::json& profile);
	std::string macAddress() const;
	std::string displayResolution() const;
	std::string productName() const;
	std::string kernelVersion() const;
	std::string volumeSerialNumber() const;
	std::string loginName() const;
	std::string language() const;
	std::string userSid() const;
	std::string routerAddress() const;
	std::string displaySize() const;
	bool isProcessBlacklisted(const std::string &name) const;

private:
	std::string generateSid() const;
	std::string generateMacAddress() const;
	std::string generateDisplaySize();
	nlohmann::json profile_{};
};
}
}