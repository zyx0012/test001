#pragma once
#include <map>
#include <vector>
#include <string>
#include <optional>
#include "../protocol/game_message/start_game.hpp"
#include "../protocol/game_message/seat_update.hpp"

namespace coinpoker {

enum TableStage {
	PREFLOP,
	FLOP,
	TURN,
	RIVER,
};

struct SeatState {
	uint8_t index{};
	std::string name{};
	std::string userId{};
	uint64_t initialStack{};
	uint64_t stack{};
	uint64_t rakeAmount{};
	std::string status{};
	std::string initialStatus{};
	bool postBlind = false;
	bool postAdditionalBlind = false;
	std::vector<std::string> holeCards{};
	uint64_t winings{};
};

struct PotState {
	uint8_t index{};
	uint64_t amount{};
	uint64_t rakeAmount{};
	std::vector<uint8_t> winners{};
};

class TableState {

public:
	uint64_t smallBlind{};
	uint64_t bigBlind{};
	uint64_t straddle{};
	uint64_t ante{};

	TableStage stage{};

	int tableSize = -1;
	int activePlayers = -1;
	int dealerIndex = -1;
	int sbIndex = -1;
	int heroIndex = -1;
	std::string mode;

	bool shouldFold = false;

	int preflopRaiseCount = 0;

	std::map<int, SeatState> seats{};
	std::vector<std::string> boardCards{};

	std::map<int, PotState> pots{};
	std::map<int, PotState> seatPots{};

	void setSeats(const std::vector<protocol::Seat>& seats);
	void updateSeat(const protocol::SeatUpdate& seat);

	std::optional<SeatState> getHero();

	bool isHeroPlaying();

	bool isBigBlind(int seatIndex);
	bool isValid();
	std::vector<SeatState> getPlayers();
	std::vector<SeatState> getPlayersAndDelar();

	int getBigBlindIndex();

	TableState() {}
};
}
