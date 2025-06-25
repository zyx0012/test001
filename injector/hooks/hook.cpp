#include "hook.hpp"
#include "../utils/utils.hpp"
#include <MinHook.h>
#include "internal_functions.hpp"
#include "websocket.hpp"
#include "../poker/table_controller.hpp"
#include "../utils/logging.hpp"
#include "../utils/config.hpp"
#include "../spoofing/spoofer.hpp"
#include "../spoofing/profile.hpp"
#include "../automation/automation.hpp"
#include "../automation/lobby.hpp"

namespace coinpoker {
namespace hook {

bool initialize(HMODULE dllInstance, utils::Logger& logger) {
	utils::attachConsole();

	wchar_t dllPath[MAX_PATH]{};
	GetModuleFileNameW(dllInstance, dllPath, MAX_PATH);
	const auto parentPath = std::filesystem::absolute(dllPath).parent_path();
	const auto configFile = parentPath / "Config.json";
	utils::InitializeLoggerSources(parentPath / "logs", parentPath / "logs-traffic");
	logger = utils::CreateLoggerNoPrefix("hooks");
	logger->info("Loading config from: {}", configFile.string());
	if (!utils::LoadConfig(configFile))
	{
		logger->error("Unable to read or parse config");
		return false;
	}
	if (MH_Initialize() != MH_OK) {
		logger->error("Failed to initialize MinHook.");
		return false;
	}
	return true;
}

struct RequiredModules {
	HMODULE qtWebSocket = nullptr;
	HMODULE qtWidgets = nullptr;
	HMODULE qtCore = nullptr;
	HMODULE qtNetwork = nullptr;
	HMODULE kernel32 = nullptr;
	HMODULE iphlpapi = nullptr;
	HMODULE user32 = nullptr;
};

std::optional<RequiredModules> ensureModulesLoaded(const utils::Logger& logger) {
	RequiredModules modules;
	modules.qtWebSocket = GetModuleHandleW(L"Qt6WebSockets.dll");
	if (!modules.qtWebSocket) {
		logger->error("Unable to find Qt6WebSockets.dll module.");
		return std::nullopt;
	}
	modules.qtCore = GetModuleHandleW(L"Qt6Core.dll");
	if (!modules.qtCore) {
		logger->error("Unable to find Qt6Core.dll module.");
		return std::nullopt;
	}
	modules.qtNetwork = GetModuleHandleW(L"Qt6Network.dll");
	if (!modules.qtNetwork) {
		logger->error("Unable to find Qt6Netowrk.dll module.");
		return std::nullopt;
	}
	modules.qtWidgets = GetModuleHandleW(L"Qt6Widgets.dll");
	if (!modules.qtWidgets) {
		logger->error("Unable to find Qt6Widgets.dll module.");
		return std::nullopt;
	}
	modules.kernel32 = GetModuleHandleW(L"kernel32.dll");
	if (!modules.kernel32) {
		logger->error("Unable to find kernel32.dll module.");
		return std::nullopt;
	}
	modules.iphlpapi = GetModuleHandleW(L"iphlpapi.dll");
	if (!modules.kernel32) {
		logger->error("Unable to find iphlpapi.dll module.");
		return std::nullopt;
	}
	modules.user32 = GetModuleHandleW(L"user32.dll");
	if (!modules.user32) {
		logger->error("Unable to find user32.dll module.");
		return std::nullopt;
	}
	return modules;
}


bool resolveInternalFunctions(const utils::Logger& logger, RequiredModules& modules, InternalFunctionTable& outInternalFns) {

	outInternalFns.networkRequestUrl = reinterpret_cast<IInternalFunctions::networkRequestUrlFunc>(
		GetProcAddress(modules.qtNetwork, "?url@QNetworkRequest@@QEBA?AVQUrl@@XZ"));
	if (!outInternalFns.networkRequestUrl) {
		logger->error("Unable to find qt function 'QNetworkRequest::url'.");
		return false;
	}
	outInternalFns.socketState = reinterpret_cast<IInternalFunctions::socketStateFunc>(
		GetProcAddress(modules.qtWebSocket, "?state@QWebSocket@@QEBA?AW4SocketState@QAbstractSocket@@XZ"));
	if (!outInternalFns.socketState) {
		logger->error("Unable to find qt function 'QNetworkRequest::url'.");
		return false;
	}


	return true;
}

bool createWebsocketHooks(const utils::Logger& logger, RequiredModules& modules, InternalFunctionTable& outInternalFns) {
	auto receiveFn = GetProcAddress(modules.qtWebSocket, "?binaryMessageReceived@QWebSocket@@QEAAXAEBVQByteArray@@@Z");
	if (!receiveFn) {
		logger->error("Unable to find function 'QWebSocket::binaryMessageReceived'.");
		return false;
	}
	auto sendFn = GetProcAddress(modules.qtWebSocket, "?sendBinaryMessage@QWebSocket@@QEAA_JAEBVQByteArray@@@Z");
	if (!sendFn) {
		logger->error("Unable to find function 'QWebSocket::sendBinaryMessage'.");
		return false;
	}

	auto execFn = GetProcAddress(modules.qtWidgets, "?exec@QApplication@@SAHXZ");
	if (!execFn) {
		logger->error("Unable to find function 'QApplication::exec'.");
		return false;
	}

	if (MH_CreateHook(
		reinterpret_cast<void*>(execFn),
		reinterpret_cast<void*>(&automation::Automation::onGuiStart),
		reinterpret_cast<void**>(&outInternalFns.applcationExec)) != MH_OK) {
		logger->error("Failed to create hook for 'QApplication::exec'.");
		return false;
	};

	if (MH_CreateHook(
		reinterpret_cast<void*>(receiveFn),
		reinterpret_cast<void*>(&Websocket::onMessageRecieve),
		reinterpret_cast<void**>(&outInternalFns.receiveBinaryMessage)) != MH_OK) {
		logger->error("Failed to create hook for 'QWebSocket::binaryMessageReceived'.");
		return false;
	};

	if (MH_CreateHook(
		reinterpret_cast<void*>(sendFn),
		reinterpret_cast<void*>(&Websocket::onMessageSent),
		reinterpret_cast<void**>(&outInternalFns.sendBinaryMessage)) != MH_OK) {
		logger->error("Failed to create hook for 'QWebSocket::sendBinaryMessage'.");
		return false;
	};
	return true;
}

BOOL setForegroundWindowHook(HWND winodw) {
	return true;
}

bool createSpooferHooks(const utils::Logger& logger, RequiredModules& modules, InternalFunctionTable& outInternalFns) {

	auto prettyProductName = GetProcAddress(modules.qtCore, "?prettyProductName@QSysInfo@@SA?AVQString@@XZ");
	if (!prettyProductName) {
		logger->error("Unable to find function 'QSysInfo::prettyProductName'.");
		return false;
	}

	if (MH_CreateHook(
		reinterpret_cast<void*>(prettyProductName),
		reinterpret_cast<void*>(&spoofing::SystemSpoofer::prettyProductName),
		reinterpret_cast<void**>(&outInternalFns.prettyProductName)) != MH_OK) {
		logger->error("Failed to create hook for 'QSysInfo::prettyProductName'.");
		return false;
	};

	auto kernelVersion = GetProcAddress(modules.qtCore, "?kernelVersion@QSysInfo@@SA?AVQString@@XZ");
	if (!kernelVersion) {
		logger->error("Unable to find function 'QSysInfo::kernelVersion'");
		return false;
	}

	if (MH_CreateHook(
		reinterpret_cast<void*>(kernelVersion),
		reinterpret_cast<void*>(&spoofing::SystemSpoofer::kernelVersion),
		reinterpret_cast<void**>(&outInternalFns.kernelVersion)) != MH_OK) {
		logger->error("Failed to create hook for 'QSysInfo::kernelVersion'");
		return false;
	}

	auto queryFullProcessImageName = GetProcAddress(modules.kernel32, "QueryFullProcessImageNameA");
	if (!queryFullProcessImageName) {
		logger->error("Unalbe to find function 'QueryFullProcessImageNameA'.");
		return false;
	}

	if (MH_CreateHook(
		reinterpret_cast<void*>(queryFullProcessImageName),
		reinterpret_cast<void*>(&spoofing::SystemSpoofer::queryFullProcessImageName),
		reinterpret_cast<void**>(&outInternalFns.queryFullProcessImageName)) != MH_OK) {
		logger->error("Failed to create hook for 'QueryFullProcessImageNameA'");
		return false;
	}

	auto networkManagerPost = GetProcAddress(modules.qtNetwork, "?post@QNetworkAccessManager@@QEAAPEAVQNetworkReply@@AEBVQNetworkRequest@@AEBVQByteArray@@@Z");
	if (!networkManagerPost) {
		logger->error("Unable to find function 'NetworkAccessManager::post'.");
		return false;
	}

	if (MH_CreateHook(
		reinterpret_cast<void*>(networkManagerPost),
		reinterpret_cast<void*>(&spoofing::SystemSpoofer::networkManagerPost),
		reinterpret_cast<void**>(&outInternalFns.networkManagerPost)) != MH_OK) {
		logger->error("Failed to create hook for 'QWebSocket::sendBinaryMessage'.");
		return false;
	};

	auto getVolumeInformationW = GetProcAddress(modules.kernel32, "GetVolumeInformationW");
	if (!getVolumeInformationW) {
		logger->error("Unable to find function 'GetVolumeInformationW'.");
		return false;
	}

	if (MH_CreateHook(
		reinterpret_cast<void*>(getVolumeInformationW),
		reinterpret_cast<void*>(&spoofing::SystemSpoofer::getVolumeInformation),
		reinterpret_cast<void**>(&outInternalFns.getVolumeInformationW)) != MH_OK) {
		logger->error("Failed to create hook for 'GetVolumeInformationW'.");
		return false;
	};

	auto getAdaptersInfo = GetProcAddress(modules.iphlpapi, "GetAdaptersInfo");
	if (!getAdaptersInfo) {
		logger->error("Unable to find function 'GetAdaptersInfo'.");
		return false;
	}

	if (MH_CreateHook(
		reinterpret_cast<void*>(getAdaptersInfo),
		reinterpret_cast<void*>(&spoofing::SystemSpoofer::getAdaptersInfo),
		reinterpret_cast<void**>(&outInternalFns.getAdaptersInfo)) != MH_OK) {
		logger->error("Failed to create hook for 'QWebSocket::sendBinaryMessage'.");
		return false;
	}

	auto hardwareAddress = GetProcAddress(modules.qtNetwork, "?hardwareAddress@QNetworkInterface@@QEBA?AVQString@@XZ");
	if (!hardwareAddress) {
		logger->error("Unable to find function 'QNetworkInterface::hardwareAddress'.");
		return false;
	}
	if (MH_CreateHook(
		reinterpret_cast<void*>(hardwareAddress),
		reinterpret_cast<void*>(&spoofing::SystemSpoofer::hardwareAddress),
		reinterpret_cast<void**>(&outInternalFns.hardwareAddress)) != MH_OK) {
		logger->error("Failed to create hook for 'QWebSocket::sendBinaryMessage'.");
		return false;
	}

	/*
	auto setForegroundWindow = GetProcAddress(modules.user32, "SetForegroundWindow");
	if (!setForegroundWindow) {
		logger->error("Unable to find function 'SetForegroundWindow'.");
		return false;
	}
	if (MH_CreateHook(
		reinterpret_cast<void*>(setForegroundWindow),
		reinterpret_cast<void*>(&setForegroundWindowHook),
		reinterpret_cast<void**>(&outInternalFns.setForegroudWindow)) != MH_OK) {
		logger->error("Failed to create hook for 'SetForegroundWindow' function.");
		return false;
	}
	*/

	return true;
}

bool createHooks(const utils::Logger& logger, RequiredModules& modules, InternalFunctionTable& outInternalFns) {
	if (!createWebsocketHooks(logger, modules, outInternalFns))
		return false;

	if (!createSpooferHooks(logger, modules, outInternalFns))
		return false;

	return true;
}

void startInjection(HMODULE dllInstance) {
	utils::Logger logger;
	if (!initialize(dllInstance, logger)) {
		return;
	}
	logger->info("Starting injection...");
	auto modules = ensureModulesLoaded(logger);
	if (!modules)
		return;
	InternalFunctionTable internalFunctionTable;
	if (!resolveInternalFunctions(logger, modules.value(), internalFunctionTable)) {
		return;
	}
	if (!createHooks(logger, modules.value(), internalFunctionTable)) {
		return;
	}
	auto internalFunctions = std::shared_ptr<IInternalFunctions>(new InternalFunctions(internalFunctionTable));

	Websocket::setInternalFunctions(internalFunctions);
	Websocket::addRecieveListener(&TableController::onNetworkPacket);
	Websocket::addSentListener(&TableController::onSendNetworkPacket);
	automation::Automation::setInternalFunctions(internalFunctions);

	spoofing::SystemSpoofer::init(internalFunctions, spoofing::Profile(utils::gConfig.profile));

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
		logger->error("Failed to enable all hooks.");
		return;
	}
	automation::Lobby::instance().start();
	logger->info("Injection succesful.");
}

void disableHooks() {
	utils::Logger logger = utils::CreateLoggerNoPrefix("hooks");
	if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK) {
		logger->error("Failed to diable hook hooks.");
		return;
	};
	logger->info("Disabled all hooks.");
	utils::detachConsole();
}

} //end hook
} //end coinpoker
