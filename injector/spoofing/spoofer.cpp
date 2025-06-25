#include "spoofer.hpp"
#include <cassert>
#include <string>
#include <nlohmann/json.hpp>
#include "../utils/utils.hpp"
#include "../utils/logging.hpp"

using json = nlohmann::json;

namespace coinpoker {
namespace spoofing {

std::shared_ptr<IInternalFunctions> SystemSpoofer::internalFunctions_ = {};
utils::Logger SystemSpoofer::logger_ = {};
Profile SystemSpoofer::profile_ = {};

void SystemSpoofer::init(std::shared_ptr<IInternalFunctions> internalFns, const Profile& profile)
{
	logger_ = utils::CreateLoggerNoPrefix("SystemSpoofer");
	internalFunctions_ = std::move(internalFns);
	profile_ = profile;
}

std::string SystemSpoofer::updateHealthRequest(const std::string& body) {
	logger_->info("Modify initHealth request body.");
	try {
		json data = json::parse(body);
		data["default_os_language"] = profile_.language();
		data["keyboard_language"] = profile_.language();
		data["display_physical_sizes"] = profile_.displaySize();
		data["display_resolutions"] = profile_.displayResolution();
		data["mac_addresses"] = profile_.macAddress();
		data["router_mac_address"] = profile_.routerAddress();
		data["os_login_name"] = profile_.loginName();
		data["os_language"] = profile_.language();
		data["vm_name"] = "Generic VM";
		data["os_security_identifier"] = profile_.userSid();
		data["volume_id"] = profile_.volumeSerialNumber();
		return data.dump();
	}
	catch (const std::exception& ex) {
		logger_->error("Failed to modify request body. Error: {}, returning original request.", ex.what());
		return body;
	}
}

void* SystemSpoofer::networkManagerPost(void* self, void* request, QByteArray* data) {
	assert(internalFunctions_ != nullptr && "Internal functions not set.");
	QUrl url;
	internalFunctions_->networkRequestUrl(request, &url);
	std::string urlString = url.toString().toStdString();
	if (urlString.find("/init/health/update") != std::string::npos) {
		std::string body(data->constData());
		std::string newBody = updateHealthRequest(body);
		QByteArray newData(newBody.c_str(), newBody.length());
		return internalFunctions_->networkManagerPost(self, request, &newData);
	}
	else {
		return internalFunctions_->networkManagerPost(self, request, data);
	}
}

BOOL SystemSpoofer::getVolumeInformation(LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer,
	DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
	LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
	LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize)
{
	if (!profile_.volumeSerialNumber().empty()) {
		*lpVolumeSerialNumber = std::stoul(profile_.volumeSerialNumber(), nullptr, 16);
		return true;
	}
	return internalFunctions_->getVolumeInformationW(
		lpRootPathName, lpVolumeNameBuffer,
		nVolumeNameSize, lpVolumeSerialNumber,
		lpMaximumComponentLength, lpFileSystemFlags,
		lpFileSystemNameBuffer, nFileSystemNameSize);
}

void SystemSpoofer::setInternalFunctions(std::shared_ptr<IInternalFunctions> fns) {
	internalFunctions_ = std::move(fns);
}

ULONG SystemSpoofer::getAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen) {
	assert(internalFunctions_ != nullptr && "Internal functions not set.");

	if (profile_.macAddress().empty()) {
		return internalFunctions_->getAdaptersInfo(pAdapterInfo, pOutBufLen);
	}

	if (pAdapterInfo == nullptr) {
		return internalFunctions_->getAdaptersInfo(pAdapterInfo, pOutBufLen);
	}
	else {
		logger_->info("Spoof macAddress to {}", profile_.macAddress());
		auto ret = internalFunctions_->getAdaptersInfo(pAdapterInfo, pOutBufLen);
		if (ret != NO_ERROR) return ret;
		utils::parseMac(profile_.macAddress(), pAdapterInfo->Address);
		pAdapterInfo->Next = nullptr;
		return ret;
	}
}

void SystemSpoofer::hardwareAddress(void* this_, QString* out) {
	auto d = profile_.macAddress().c_str();
	internalFunctions_->hardwareAddress(this_, out);
	out->assign(d);
}

QString SystemSpoofer::prettyProductName() {
	return QString::fromStdString(profile_.productName());
}

QString SystemSpoofer::kernelVersion() {
	return QString::fromStdString(profile_.kernelVersion());
}

BOOL SystemSpoofer::queryFullProcessImageName(HANDLE hProcess, DWORD dwFlags, LPSTR lpExeName, PDWORD lpdwSize) {
	assert(internalFunctions_ != nullptr && "Internal functions not set.");
	BOOL ret = internalFunctions_->queryFullProcessImageName(hProcess, dwFlags, lpExeName, lpdwSize);
	std::string processName(lpExeName);
	if (!profile_.isProcessBlacklisted(processName)) return ret;
	logger_->info("Hide blocked process: {}", processName);
	const char* realName = "C:\\Windows\\System32\\notepad.exe";
	if ((*lpdwSize) > strlen(realName)) {
		strcpy_s(lpExeName, *lpdwSize, realName);
		*lpdwSize = strlen(realName);
	}
	else {
		strncpy_s(lpExeName, *lpdwSize, realName, *lpdwSize - 1);
		lpExeName[*lpdwSize - 1] = '\0';
	}
	return ret;
}


}
}