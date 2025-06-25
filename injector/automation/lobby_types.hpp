#pragma once
#include <string>

namespace coinpoker::automation {

enum GameType {
	Invalid = -1,
	Holdem = 1,
	Omaha = 2,
	FiveOmaha = 16
};

enum TableType {
	HeadsUp = 0x00000001,
	Max3Players = 0x00000002,
	FiveCardOmaha = 0x00000003,
	Max4Players = 0x00000004,
	Max5Players = 0x00000008,
	Max6Players = 0x00000010,
	Cage = 0x00000011,
	Rebuy = 0x00000020,
	DeepStack = 0x00000040,
	ShortStack = 0x00000080,
	ShortDeck = 0x00000100,
	Turbo = 0x00000200,
	MobileOnly = 0x00000400,
	HyperTurbo = 0x00000800,
	Satellite = 0x00001000,
	Freeroll = 0x00002000,
	DisconnectProtection = 0x00004000,
	KnockoutBounty = 0x00008000,
	Shootout = 0x00010000,
	SpecialHS = 0x00020000,
	PlaySets = 0x00040000,
	NewGame = 0x00080000,
	Capped = 0x00100000,
	RunItTwice = 0x00200000,
	DeepStack2 = 0x00400000,
	ShortStack2 = 0x00800000,
	Ante = 0x01000000,
	Special = 0x02000000,
	Private = 0x04000000,
	Freezout = 0x08000000,
	PotLimitOmaha = 0x10000000,
	GTD = 0x20000000,
	Freebuy = 0x40000000,
	RNG = 0x80000000,
};

struct UserData {
	uint32_t userId;
	uint32_t balance;
	std::string username;
	std::string email;
	std::string ssid;
};

enum class CurrentState {
	OPENING,
	OPENED,
	CLOSED,
	SITTING_OUT,
	WAITING,
	PLAYING,
	ALONE,
};

enum class CloseReason {
	NORMAL,
	COOLDOWN,
	WAITING
};

}
