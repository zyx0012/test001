#pragma once
#include "hook.hpp"
#include <cstddef> 
#include <iphlpapi.h>
#include "QtCore/qbytearray.h"
#include "QtCore/qstring.h"
#include "QtCore/qurl.h"

namespace coinpoker {

typedef void* QWebSocketPtr;

#define INTERNAL_FUNCTION_LIST \
    FUNC(receiveBinaryMessage, int, (QWebSocketPtr self, QByteArray* input), (self, input)) \
    FUNC(sendBinaryMessage, long long, (QWebSocketPtr self, QByteArray* input), (self, input)) \
    FUNC(socketState, int, (QWebSocketPtr self), (self)) \
    FUNC(networkManagerPost, void*, (void* self,void*  request, void* data), (self, request, data)) \
    FUNC(networkRequestUrl, void, (void* self, QUrl* out), (self, out))  /*value returned by hidden pointer in RDX*/ \
	FUNC(getVolumeInformationW, BOOL, \
    (LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, \
     LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength, \
     LPDWORD lpFileSystemFlags, LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize), \
    (lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize, \
     lpVolumeSerialNumber, lpMaximumComponentLength, \
     lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize)) \
	FUNC(getAdaptersInfo, ULONG, (PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen), (pAdapterInfo, pOutBufLen)) \
	FUNC(hardwareAddress, void, (void* self, QString* out), (self, out)) \
	FUNC(prettyProductName, void, (QString* out), (out)) \
	FUNC(kernelVersion, void, (QString* out), (out)) \
	FUNC(queryFullProcessImageName, BOOL, (HANDLE hProcess,DWORD dwFlags, LPSTR lpExeName, PDWORD lpdwSize), (hProcess, dwFlags, lpExeName, lpdwSize)) \
	FUNC(applcationExec, int, (), ()) \
	FUNC(setForegroudWindow, BOOL, (HWND w), (w)) \

class IInternalFunctions {
public:
#define FUNC(name, retType, args, params) using name##Func = retType (*) args;
	INTERNAL_FUNCTION_LIST
#undef FUNC

#define FUNC(name, ret, args, params) \
    virtual ret __fastcall name args = 0;
		INTERNAL_FUNCTION_LIST
#undef FUNC
		virtual ~IInternalFunctions() = default;
};

struct InternalFunctionTable {
#define FUNC(name, ret, args, params) IInternalFunctions::name##Func name;
	INTERNAL_FUNCTION_LIST
#undef FUNC
};


class InternalFunctions : public IInternalFunctions {
public:
	InternalFunctions(const InternalFunctionTable& funcs) : funcs(funcs) {}

#define FUNC(name, retType, args, params) retType __fastcall name args override { return funcs.name params; };
	INTERNAL_FUNCTION_LIST
#undef FUNC

private:
	InternalFunctionTable funcs;
};
}  // namespace coinpoker
