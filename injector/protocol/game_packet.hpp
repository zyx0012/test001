#pragma once
#include "protocol_header.hpp"
#include "../utils/utils.hpp"
#include <string>
#include <variant>
#include <chrono>

namespace coinpoker {
namespace protocol {

enum class MessageIds {
	OnGameStart = 10001,
	OnOpenTable = 10002,
	OnAction = 10007,
	OnSeatUpdate = 10003,
	OnEnterTable = 10008,
	OnDealHoleCards = 10017,
	OnDealHeroCards = 10011,
	OnDealBoardCards = 10019,
	OnNextTurn = 10006,
	OnHeroTurn = 10010,
	OnPotRakeUpdate = 10023,
	OnPotUpdate = 10022,
	OnGameEnd = 10035,
	OnBuyIn = 10012,
};

class GamePacket {

private:
	Header header;
	std::variant<std::string_view, std::string> body;
	std::chrono::system_clock::time_point timestamp;

public:
	GamePacket(Header header, std::string body)
		: header(header), body(std::move(body)), timestamp(std::chrono::system_clock::now()) {}

	GamePacket(Header header, std::string_view body)
		: header(header), body(body), timestamp(std::chrono::system_clock::now()) {}

	Header getHeader() const { return header; };

	const std::string getBody() const {
		if (std::holds_alternative<std::string>(body)) {
			return std::get<std::string>(body);
		}
		return std::string(std::get<std::string_view>(body));
	}


	std::chrono::system_clock::time_point getTimestamp() const {
		return timestamp;
	}

	bool ownsBody() const {
		return std::holds_alternative<std::string>(body);
	}

	void takeOwnership() {
		if (!ownsBody()) {
			body = std::string(std::get<std::string_view>(body));
		}
	}

	int getMessageId() const { return header.msgId; };
	int getInstanceId() const { return header.instanceId; };

	

	bool isInteresting() {
		switch (static_cast<protocol::MessageIds>(this->getMessageId())) {
		case protocol::MessageIds::OnGameStart:
		case protocol::MessageIds::OnAction:
		case protocol::MessageIds::OnSeatUpdate:
		case protocol::MessageIds::OnEnterTable:
		case protocol::MessageIds::OnDealHoleCards:
		case protocol::MessageIds::OnDealHeroCards:
		case protocol::MessageIds::OnDealBoardCards:
		case protocol::MessageIds::OnNextTurn:
		case protocol::MessageIds::OnHeroTurn:
		case protocol::MessageIds::OnPotRakeUpdate:
		case protocol::MessageIds::OnPotUpdate:
		case protocol::MessageIds::OnGameEnd:
		case protocol::MessageIds::OnOpenTable:
		case protocol::MessageIds::OnBuyIn:
			return true;
		default:
			return false;
		}
	}
};

class GamePacketRequest {

private:
	SendHeader header{};
	std::string body{};
	std::string messageType{};
	std::vector<std::string> bodyParts{};

public:
	GamePacketRequest(SendHeader header, std::string body)
		: header(header), body(body) {
		bodyParts = utils::splitString(body, '\t');
		messageType = bodyParts[0];
	}
	SendHeader getHeader() const { return header; };

	std::string getBody() const { return body; };

	std::vector<std::string> getBodyParts() const { return bodyParts; };

	std::string getType() const {
		return messageType;
	};

	int getInstanceId() const { return header.instanceId; };
	int getTableId() const { return header.tableId; };

	std::vector<char> prepare() {
		std::vector<char> buffer(sizeof(SendHeader) + body.length() + 1);

		std::memcpy(buffer.data(), &header, sizeof(SendHeader));
		std::memcpy(buffer.data() + sizeof(SendHeader), body.data(), body.length());
		buffer.back() = '\0';

		return buffer;
	}

	bool isInteresting() {
		if (messageType == "join") return true;
		if (messageType == "leave") return true;
		if (messageType == "option") return true;
		return false;
	}
};

}
}

