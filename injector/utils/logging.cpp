#include "logging.hpp"
#include <string>
#include <format>
#include <random>
#include <limits>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/details/os.h>
#include <windows.h>
#include <iostream>

namespace coinpoker {
namespace utils {
namespace {
static std::string GetLogFilename()
{
	const auto tm = spdlog::details::os::localtime();
	return std::format("{:04}-{:02}-{:02}_{:02}-{:02}-{:02}.log", // YYYY-MM-DD_HH-MM-SS.log
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
}
static std::string GetTrafficLogFilename()
{
	const auto tm = spdlog::details::os::localtime();
	return std::format("{:04}-{:02}-{:02}_{:02}-{:02}-{:02}_traffic.log", // YYYY-MM-DD_HH-MM-SS.log
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
}
static auto& GetStdoutFileSinks(std::string_view path)
{
	static std::vector<spdlog::sink_ptr> sinks{
		std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>(),
		std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.data(), true)
	};

	return sinks;
}
}  // namespace

LoggerSource CreateLogger{};
LoggerSource CreateLoggerNoPrefix{};
LoggerSource CreateTrafficLogger{};

void InitializeLoggerSources(
	const std::filesystem::path& logs,
	const std::filesystem::path& trafficLogs
)
{
	CreateLogger = [file = logs / GetLogFilename()](std::string_view prefix)
		{
			auto& sinks = GetStdoutFileSinks(file.string());

			static std::random_device device;
			static std::mt19937 generator(device());
			std::uniform_int_distribution<uint32_t> distribution(
				(std::numeric_limits<uint32_t>::min)(),
				(std::numeric_limits<uint32_t>::max)()
			);

			// Append a random number to the end to uniquely identify this logger
			const std::string name = std::format("{}[{:08X}]", prefix, distribution(generator));

			Logger logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
			logger->set_pattern("%^%Y-%m-%d %T TID=%-4t %8l [%n] %v%$");
			logger->set_level(spdlog::level::trace);
			logger->flush_on(spdlog::level::trace);
			return logger;
		};

	CreateLoggerNoPrefix = [file = logs / GetLogFilename()](std::string_view name)
		{
			auto& sinks = GetStdoutFileSinks(file.string());

			Logger logger = std::make_shared<spdlog::logger>(name.data(), sinks.begin(), sinks.end());
			logger->set_pattern("%^%Y-%m-%d %T TID=%-4t %8l [%n] %v%$");
			logger->set_level(spdlog::level::trace);
			logger->flush_on(spdlog::level::trace);
			return logger;
		};

	CreateTrafficLogger = [file = trafficLogs / GetTrafficLogFilename()](std::string_view direction)
		{
			static const std::vector<spdlog::sink_ptr> sinks{
				std::make_shared<spdlog::sinks::basic_file_sink_mt>(file.string(), true)
			};

			Logger logger = std::make_shared<spdlog::logger>(direction.data(), sinks.begin(), sinks.end());
			logger->set_pattern("%^%Y-%m-%d %T %4n %v%$");
			logger->set_level(spdlog::level::trace);
			logger->flush_on(spdlog::level::trace);
			return logger;
		};

	// Create a global logger
	spdlog::set_default_logger(CreateLogger("default"));
	spdlog::set_error_handler([](const std::string& msg) {
		std::cerr << "SPDLOG ERROR: " << msg << std::endl;
		});
};
}  // namespace utils
}
