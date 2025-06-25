#include <spdlog/spdlog.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <thread>

#include <Windows.h>
#include <TlHelp32.h>
#include <Shlobj.h>
#include "../injector/utils/config.hpp"
#include <winhttp.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32.lib")

using namespace std::chrono_literals;

#undef PROCESSENTRY32
#undef Process32First
#undef Process32Next

static constexpr std::string_view kStartupValueName{ "WatchdogForCoinPoker" };
static constexpr std::string_view kDefaultDllName{ "injector.dll" };

// Gets the ProcessID from a process name, modules is an optional parameter that will only return after certain modules are loaded
static DWORD GetProcessIdFromName(std::string_view name)
{
	const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32 entry{ sizeof(PROCESSENTRY32) };

	for (Process32First(snapshot, &entry); Process32Next(snapshot, &entry);)
	{
		if (name == entry.szExeFile)
		{
			CloseHandle(snapshot);
			return entry.th32ProcessID;
		}
	}

	CloseHandle(snapshot);
	return 0;
}

std::string getPublicIP() {
	HINTERNET session = WinHttpOpen(L"WinHttp/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session) return "";

	HINTERNET connect = WinHttpConnect(session, L"api.ipify.org",
		INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!connect) {
		WinHttpCloseHandle(session);
		return "";
	}

	HINTERNET request = WinHttpOpenRequest(connect, L"GET", NULL,
		NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		WINHTTP_FLAG_SECURE);
	if (!request) {
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
		return "";
	}

	BOOL success = WinHttpSendRequest(request,
		WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
		WinHttpReceiveResponse(request, NULL);
	if (!success) {
		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
		return "";
	}

	std::string result;
	DWORD size = 0;
	while (WinHttpQueryDataAvailable(request, &size) && size > 0) {
		std::string buffer(size, '\0');
		DWORD read = 0;
		if (!WinHttpReadData(request, &buffer[0], size, &read)) break;
		buffer.resize(read);
		result += buffer;
	}

	WinHttpCloseHandle(request);
	WinHttpCloseHandle(connect);
	WinHttpCloseHandle(session);
	return result;
}

std::string resolveVmHost(const std::string& hostname) {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return {};

	addrinfo hints = {}, * res = nullptr;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(hostname.c_str(), nullptr, &hints, &res) != 0) {
		WSACleanup();
		return {};
	}

	char ipStr[INET_ADDRSTRLEN] = {};
	if (res) {
		sockaddr_in* ipv4 = (sockaddr_in*)res->ai_addr;
		inet_ntop(AF_INET, &(ipv4->sin_addr), ipStr, sizeof(ipStr));
	}

	freeaddrinfo(res);
	WSACleanup();
	return ipStr;
}

struct SharedConfig {
	int64_t sessionStartTime;
};

void writeSharedConfig(const SharedConfig& data) {
	HANDLE hMapFile = CreateFileMappingA(
		INVALID_HANDLE_VALUE, nullptr,
		PAGE_READWRITE,
		0, sizeof(SharedConfig),
		"Local\\CoinPokerWatchdogConfig"
	);

	if (!hMapFile) return;

	void* pBuf = MapViewOfFile(hMapFile, FILE_MAP_WRITE, 0, 0, sizeof(SharedConfig));
	if (!pBuf) {
		CloseHandle(hMapFile);
		return;
	}

	memcpy(pBuf, &data, sizeof(SharedConfig));

	UnmapViewOfFile(pBuf);
}

bool isUsingVPN() {
	auto publicIp = getPublicIP();
	if (publicIp.empty()) {
		spdlog::error("Failed to get public ip.");
		return false;
	}
	auto vmIp = resolveVmHost(coinpoker::utils::gConfig.vmHost);
	if (vmIp.empty()) {
		spdlog::error("Failed to resolve ip for {}.", coinpoker::utils::gConfig.vmHost);
		return false;
	}
	if (publicIp == vmIp) {
		spdlog::error("VM not connected to VPN.");
		return false;
	}
	return true;
}
// Adds the current EXE to launch at computer startup.
// NOTE: This only affects the currently logged in user.
static void EnsureExeRunsAtStartup()
{
	char filename[MAX_PATH]{};
	GetModuleFileNameA(GetModuleHandleA(nullptr), filename, sizeof filename);

	const auto dir = std::filesystem::path{ filename }.parent_path() / kDefaultDllName;
	const std::string commandLine = std::format("\"{}\" \"{}\"", filename, dir.string());

	HKEY key{};
	RegCreateKeyA(HKEY_CURRENT_USER, R"(Software\Microsoft\Windows\CurrentVersion\Run)", &key);
	RegSetValueExA(key, kStartupValueName.data(), 0, REG_SZ,
		reinterpret_cast<const BYTE*>(commandLine.data()), commandLine.size());

	RegCloseKey(key);
}

// Injects a DLL located at filepath into the process designated processId.
static bool RemoteInjection(uint32_t processId, const std::wstring_view filepath)
{
	if (!isUsingVPN()) return false;
	const size_t filepathSize = filepath.size() * sizeof(decltype(filepath)::value_type);

	const std::unique_ptr<void, decltype(&CloseHandle)> handle{ OpenProcess(PROCESS_ALL_ACCESS, false, processId), &CloseHandle };
	if (!handle.get())
		return false;

	const PVOID pathLocation = VirtualAllocEx(handle.get(), nullptr, filepathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!pathLocation)
		return false;

	if (!WriteProcessMemory(handle.get(), pathLocation, filepath.data(), filepathSize, nullptr))
		return false;

	const FARPROC llw = GetProcAddress(GetModuleHandleA("kernel32"), "LoadLibraryW");

	for (size_t i{}; i < 500; ++i)
	{
		const std::unique_ptr<void, decltype(&CloseHandle)> thread{
		CreateRemoteThread(handle.get(), nullptr, 0, reinterpret_cast<PTHREAD_START_ROUTINE>(llw), pathLocation, 0, nullptr), &CloseHandle };
		if (!thread.get())
			return false;

		// Wait for that thread to finish execution.
		WaitForSingleObject(thread.get(), INFINITE);

		std::this_thread::sleep_for(std::chrono::milliseconds{ 2 });
	}

	VirtualFreeEx(handle.get(), pathLocation, 0, MEM_RELEASE);

	return true;
}

static std::filesystem::path
GetModuleParentPath(HMODULE instance)
{
	wchar_t dllPath[MAX_PATH]{};
	GetModuleFileNameW(instance, dllPath, MAX_PATH);

	return std::filesystem::absolute(dllPath).parent_path();
}

static void
RogueLaunchWatchdog(const std::filesystem::path& internalPath)
{
	// The last process id we know of
	uint32_t lastProcessId{};

	while (true)
	{
		using namespace std::chrono_literals;

		const uint32_t processId = GetProcessIdFromName("game.exe");
		if (!processId)
		{
			std::this_thread::sleep_for(100ms);
			continue;
		}

		if (processId == lastProcessId)
		{
			std::this_thread::sleep_for(1s);
			continue;
		}

		if (RemoteInjection(processId, internalPath.wstring()))
			spdlog::info("Injection into ProcessId={} succeeded", processId);
		else
			spdlog::error("Injection into ProcessId={} failed", processId);

		lastProcessId = processId;
	}
}

int main(int argumentCount, const char* argumentVector[])
{
	EnsureExeRunsAtStartup();

	const auto parentPath = GetModuleParentPath(nullptr);

	spdlog::info("===== Watchdog for CoinPoker =====");
	if (!coinpoker::utils::LoadConfig("Config.json")) {
		spdlog::error("Failed to load config.");
		return -1;
	}

	HANDLE hShutdown = CreateEventA(nullptr, TRUE, FALSE, "Local\\CoinPokerWatchdogShutdown");
	if (!hShutdown) {
		spdlog::error("Failed to create shutodown event.");
		return -1;
	}

	// Grab the absolute file path for the Internal.dll on disk.
	const std::filesystem::path internalPath = argumentCount == 2 ? argumentVector[1] : std::filesystem::absolute(kDefaultDllName);
	if (!std::filesystem::exists(internalPath))
	{
		MessageBoxA(nullptr, std::format("{}\n\ncould not be located", internalPath.string()).data(), "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return -1;
	}

	using clock = std::chrono::high_resolution_clock;

	// Game start and injection sequence
	const auto startAndInject = [&]
		{
			if (!isUsingVPN()) {
				return;
			}

			if (GetProcessIdFromName("game.exe"))
			{
				// Process is already open
				return;
			}

			std::system(R"(start "" /D "C:\CoinPoker" "C:\CoinPoker\Lobby.exe")");

			const auto startTime = clock::now();

			// Give up to 1 minute before stopping to look for the process
			while (std::chrono::duration_cast<std::chrono::seconds>(clock::now() - startTime) < 60s)
			{
				const uint32_t processId = GetProcessIdFromName("game.exe");
				if (!processId) {
					continue;
				}

				if (RemoteInjection(processId, internalPath.wstring())) {
					spdlog::info("Injection into ProcessId={} succeeded", processId);
				}
				else {
					spdlog::error("Injection into ProcessId={} failed", processId);
				}

				break;
			}
		};


	writeSharedConfig(
		SharedConfig{
			.sessionStartTime = std::chrono::system_clock::now().time_since_epoch().count()
		}
	);
	// CoinPoker may be launched "rogue" - not by this process and instead by the user directly or by update
	// We need anothet thread constantly watching for this scenario to ensure we never launch the game without injection.
	std::thread{ RogueLaunchWatchdog, internalPath }.detach();

	spdlog::info("Waiting 20s before we auto launch the game...");
	std::this_thread::sleep_for(20s);
	

	startAndInject();


	while (true)
	{
		if (WaitForSingleObject(hShutdown, 0) == WAIT_OBJECT_0)
		{
			spdlog::info("Shutdown signal received - exiting monitor.");
			break;
		}

		const uint32_t processId = GetProcessIdFromName("game.exe");
		if (processId != 0)
		{
			// Game is running — wait for 1s and check again
			std::this_thread::sleep_for(1s);
			continue;
		}
		if (WaitForSingleObject(hShutdown, 0) == WAIT_OBJECT_0)
		{
			spdlog::info("Shutdown signal received - exiting monitor.");
			break;
		}

		// Game closed or crashed, start a new instance
		spdlog::info("Crash detected, waiting 10s before we launch again...");
		std::this_thread::sleep_for(10s);
		ResetEvent(hShutdown);
		startAndInject();
	}

	return 0;
}