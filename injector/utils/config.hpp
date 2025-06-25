#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <fstream>
#include <string>
#include <filesystem>

namespace coinpoker {
namespace utils {


struct Stakes {
	float sb;
	float bb;
	float ante;
};

struct ActionDelay {
	float min;
	float max;
};

struct TableConfig
{
	uint32_t tableCount{};
	uint8_t tableSize{};
	Stakes stakes{};
	bool emptyTable;
};

struct Config
{
	std::string username{};
	std::string password{};
	std::string    aiApiAddress{};
	uint16_t       aiApiPort{};
	bool           autoSitIn{};
	bool           waitForBB{};
	nlohmann::json profile{};
	std::vector<TableConfig> tables{};
	uint32_t maxStackBB;
	ActionDelay preflopActionDelay;
	ActionDelay preflopActionRaiseDelay;
	ActionDelay flopActionDelay;
	ActionDelay turnActionDelay;
	ActionDelay riverActionDelay;
	uint32_t tableCooldown;
	uint32_t sessionLength;
	uint32_t breaks;
	uint32_t breaksDuration;
	std::string vmHost;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ActionDelay
	, min
	, max
);


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Stakes
	, sb
	, bb
	, ante
);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TableConfig
	, tableCount
	, tableSize
	, stakes
	, emptyTable
);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config
	, username
	, password
	, aiApiAddress
	, aiApiPort
	, profile
	, autoSitIn
	, waitForBB
	, tables
	, maxStackBB
	, preflopActionDelay
	, flopActionDelay
	, turnActionDelay
	, riverActionDelay
	, tableCooldown
	, sessionLength
	, breaks
	, breaksDuration
	, vmHost
);

inline Config gConfig{};

inline bool LoadConfig(const std::filesystem::path& path)
{
	std::ifstream configStream{ path };

	const auto json = nlohmann::json::parse(configStream, nullptr, false);
	if (json.is_discarded()) {
		return false;
	}

	try {
		utils::gConfig = json;
	}
	catch (std::exception&) {
		return false;
	}

	return true;
}
}  // namespace utils
}  // coinpoker
