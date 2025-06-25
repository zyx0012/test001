#pragma once
#include <string>
#include <format>

namespace coinpoker {
namespace protocol {

class TableOption {
public:
	uint32_t tableId{};
	bool autoPostAnte{};
	bool autoPostBlinds{};
	bool autoMuckWinning{};
	bool autoMuckLosing{};
	bool askForStraddle{};
	bool runBoardTwice{};
	bool waitForBB{};
	bool vrg{};
	bool autoTimeBank{};

	std::string serialize() {
		return std::format(
			"option\t{}\tAPA\t{}\tAPB\t{}\tAMW\t{}\tAML\t{}\tSDL\t{}\tR2X\t{}\tWBB\t{}\tVRG\t{}\tATB\t{}",
			tableId, static_cast<int>(autoPostAnte), static_cast<int>(autoPostBlinds),
			static_cast<int>(autoMuckWinning), static_cast<int>(autoMuckLosing), static_cast<int>(askForStraddle),
			static_cast<int>(runBoardTwice), static_cast<int>(waitForBB), static_cast<int>(vrg), static_cast<int>(autoTimeBank)
		);
	}
};

}
}

