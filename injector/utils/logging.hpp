#pragma once

#include <spdlog/spdlog.h>

#include <filesystem>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <functional>

namespace coinpoker {
namespace utils {
using Logger = std::shared_ptr<spdlog::logger>;
using LoggerSource = std::function<Logger(std::string_view)>;

extern LoggerSource CreateLogger;
extern LoggerSource CreateLoggerNoPrefix;
extern LoggerSource CreateTrafficLogger;

void InitializeLoggerSources(
	const std::filesystem::path& logs,
	const std::filesystem::path& trafficLogs
);
}  // namespace utils
};
