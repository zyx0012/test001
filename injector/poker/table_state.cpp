#include "table_state.hpp"

namespace coinpoker {
void TableState::setSeats(const std::vector<protocol::Seat>& seats) {
	for (const protocol::Seat& seat : seats) {
		SeatState s;
		s.index = seat.index;
		s.name = seat.name;
		s.stack = seat.stack;
		s.initialStack = seat.stack;
		s.status = seat.status;
		s.initialStatus = seat.status;
		s.userId = seat.playerId;
		this->seats[seat.index] = s;
	}
}

void TableState::updateSeat(const protocol::SeatUpdate& seat) {
	auto it = seats.find(seat.index);
	if (it == seats.end()) return;
	auto& player = it->second;
	player.stack = seat.stack;
	player.status = seat.status;
}

std::optional<SeatState> TableState::getHero() {
	if (heroIndex == -1) return std::nullopt;
	auto it = seats.find(heroIndex);
	if (it == std::end(seats)) return std::nullopt;
	return it->second;
}

bool TableState::isBigBlind(int seatIndex) {
	auto players = getPlayers();
	if (players.size() < 2) return false;
	return players[1].index == seatIndex;
}

int TableState::getBigBlindIndex() {
	auto players = getPlayers();
	if (players.size() < 2) return -1;
	return players[1].index;
}

bool TableState::isHeroPlaying() {
	auto hero = getHero();
	if (!hero) return false;
	return hero->status == "T";
}

std::vector<SeatState> TableState::getPlayers() {
	std::map<int, SeatState> playersSb{};
	for (auto& [index, player] : seats) {
		int pos = (index - sbIndex + tableSize) % tableSize;
		playersSb[pos] = player;
	}
	std::vector<SeatState> players;
	for (const auto& [pos, player] : playersSb) {
		if (player.initialStatus == "T") {
			players.push_back(player);
		}
	}
	return players;
}

std::vector<SeatState> TableState::getPlayersAndDelar() {
	std::map<int, SeatState> playersSb{};
	for (auto& [index, player] : seats) {
		int pos = (index - sbIndex + tableSize) % tableSize;
		playersSb[pos] = player;
	}
	std::vector<SeatState> players;
	for (const auto& [pos, player] : playersSb) {
		if (player.initialStatus == "T" || player.index == dealerIndex) {
			players.push_back(player);
		}
	}
	return players;
}


bool TableState::isValid() {
	return mode == "state37";
}

}
