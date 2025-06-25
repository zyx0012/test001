#pragma once
#include <cstdint>

namespace coinpoker {
namespace protocol {
	
#pragma pack(push, 1)
struct Header {
	uint32_t operationId;
	int msgId;
	int instanceId;
	int msgCounter;
	int unknown;
	uint32_t ip;
	uint16_t port;
	short length;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SendHeader {
	uint32_t operationId;
	uint32_t tableId;
	uint32_t instanceId;
	uint32_t serverTarget;
	uint64_t unused;
	uint16_t unused1;
	uint16_t length;
};
#pragma pack(pop)
};
};
