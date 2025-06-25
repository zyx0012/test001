#include "./profile.hpp"
#include <string>
#include <format>
#include <random>
#include <regex>

namespace coinpoker {
namespace spoofing {

Profile::Profile(const nlohmann::json& profile) : profile_(profile) {
	profile_["sp"]["router_address"] = generateMacAddress();
	profile_["sp"]["user_sid"] = generateSid();
	profile_["sp"]["physical_display_size"] = generateDisplaySize();
};

std::string Profile::macAddress() const {
	return profile_["sp"]["mac_address"].get<std::string>();
}

std::string Profile::displayResolution() const {
	auto width = profile_["sp"]["display_res"][0].get<uint32_t>();
	auto height = profile_["sp"]["display_res"][1].get<uint32_t>();
	return std::to_string(width) + "x" + std::to_string(height);
}

std::string Profile::productName() const {
	auto buildNumber = std::stoi(profile_["template"]["operating_system"]["BuildNumber"].get<std::string>());
	if (buildNumber >= 26100) {
		return "Windows 11 Version 24H2";
	}
	if (buildNumber >= 22631) {
		return "Windows 11 Version 23H2";
	}
	if (buildNumber >= 22621) {
		return "Windows 11 Version 22H2";
	}
	if (buildNumber >= 22000) {
		return "Windows 11 Version 21H2";
	}
	return "Windows 11 Version 24H2";
}

std::string Profile::kernelVersion() const {
	return profile_["template"]["operating_system"]["Version"].get<std::string>();
}

std::string Profile::volumeSerialNumber() const {
	auto serialNumber = profile_["template"]["volume_c"]["SerialNumber"].get<uint32_t>();
	return std::format("{:X}", serialNumber);
}

std::string Profile::language() const {
	return profile_["sp"]["language"].get<std::string>();
}

std::string Profile::loginName() const {
	return profile_["sp"]["user_name"].get<std::string>();
}

std::string Profile::userSid() const {
	return profile_["sp"]["user_sid"].get<std::string>();
}

std::string Profile::routerAddress() const {
	return profile_["sp"]["router_address"].get<std::string>();
}

std::string Profile::displaySize() const {
	return profile_["sp"]["physical_display_size"].get<std::string>();
}

bool Profile::isProcessBlacklisted(const std::string &name) const {
	for (auto &&node : profile_["process_blacklist"])
	{
		const std::regex rgx {node.get<std::string>()};
		if (std::smatch sm {}; std::regex_match(name, sm, rgx))
			return true;
	}

	return false;
}


std::string Profile::generateDisplaySize() {
    std::vector<std::string> displays = {
        "294x166", // 13.3"
        "310x174", // 14.0"
        "344x194", // 15.6"
        "353x199", // 16.0"
        "382x215"  // 17.3"
		"477x268", // 21.5"
        "509x286", // 23.8"
        "527x296", // 24"
        "598x336", // 27"
        "708x398", // 32"
    };

    static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<> dist(0, static_cast<int>(displays.size()) - 1);
    return displays[dist(rng)];
}

std::string Profile::generateSid() const {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dist(1000000000, 4000000000);
	std::uniform_int_distribution<uint32_t> ridDist(1000, 1100);

	std::string sid = "S-1-5-21";
	for (int i = 0; i < 3; ++i) {
		sid += "-" + std::to_string(dist(gen));
	}
	sid += "-" + std::to_string(ridDist(gen));
	return sid;
}

std::string Profile::generateMacAddress() const {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(0x00, 0xFF);
	std::ostringstream mac;

	for (int i = 0; i < 6; ++i) {
		int byte = dist(gen);

		if (i == 0) {
			byte &= 0xFE;
			byte |= 0x02;
		}

		mac << std::hex << std::setw(2) << std::setfill('0') << byte;
		if (i != 5) mac << ":";
	}
	return mac.str();
}
}
}