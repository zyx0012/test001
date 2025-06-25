#pragma once
#include "../hooks/internal_functions.hpp" 
#include "../utils/config.hpp" 
#include "../utils/logging.hpp" 
#include "./profile.hpp" 
#include "QtCore/qstring.h" 
#include <memory>

namespace coinpoker {
namespace spoofing {
class SystemSpoofer {
public:
	static void* networkManagerPost(void* self, void* request, QByteArray* data);
	static BOOL getVolumeInformation(LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer,
		DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber,
		LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags,
		LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize);

	static ULONG getAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);
	static void setInternalFunctions(std::shared_ptr<IInternalFunctions>);
	static void hardwareAddress(void* this_, QString* out);
	static QString prettyProductName();
	static QString kernelVersion();
	static BOOL queryFullProcessImageName(HANDLE hProcess, DWORD dwFlags, LPSTR lpExeName, PDWORD lpdwSize);

	static void init(std::shared_ptr<IInternalFunctions> internalFns, const Profile& profile);

private:
	static std::string updateHealthRequest(const std::string& body);

	static std::shared_ptr<IInternalFunctions> internalFunctions_;
	static utils::Logger logger_;
	static Profile profile_;
};
}
}