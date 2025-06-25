#include "websocket.hpp"
#include "../protocol/protocol_header.hpp"
#include "../protocol/game_packet.hpp"
#include "../protocol/stream_reader.hpp"
#include "../protocol/game_message/lobby_data.hpp"
#include "../automation/lobby.hpp"

namespace coinpoker {

std::shared_ptr<IInternalFunctions> Websocket::qtFunctions = {};
std::vector<Websocket::Listener> Websocket::recieveListeners = {};
std::vector<Websocket::SendListener> Websocket::sentListeners = {};
std::unordered_map<QWebSocketPtr, std::shared_ptr<Websocket>> Websocket::sockets = {};

void Websocket::addRecieveListener(const Websocket::Listener& listener) {
	recieveListeners.push_back(listener);
}

void Websocket::addSentListener(const Websocket::SendListener& listener) {
	sentListeners.push_back(listener);
}

int  Websocket::onMessageRecieve(QWebSocketPtr webSocket, QByteArray* buffer) {
	if (!qtFunctions) {
		return 0;
	}
	int ret = qtFunctions->receiveBinaryMessage(webSocket, buffer);
	int size = buffer->size();
	const char* data = buffer->constData();
	std::shared_ptr<Websocket> ws = getSocket(webSocket);
	handleMessage(ws, data, size);
	return ret;
}

long long Websocket::onMessageSent(QWebSocketPtr webSocket, QByteArray* buffer) {
	if (!qtFunctions) {
		return 0;
	}
	int size = buffer->size();
	const char* data = buffer->constData();
	std::shared_ptr<Websocket> ws = getSocket(webSocket);
	std::optional<protocol::GamePacketRequest> modifiedMessage = handleSentMessage(ws, data, size);
	if (modifiedMessage.has_value()) {
		return sendMessage(ws, modifiedMessage.value());
	}
	else {
		return qtFunctions->sendBinaryMessage(webSocket, buffer);
	}
}

std::shared_ptr<Websocket> Websocket::getSocket(QWebSocketPtr webSocket) {
	auto [it, exists] = sockets.try_emplace(webSocket, std::make_shared<Websocket>(webSocket));
	return it->second;
}

std::optional<protocol::GamePacketRequest> Websocket::handleSentMessage(std::shared_ptr<Websocket> ws, const char* data, int size) {
	if (size < 28) return std::nullopt;
	const auto* const header = reinterpret_cast<const protocol::SendHeader*>(data);
	if (header->operationId == 0x80000001) return std::nullopt;
	if (header->operationId == 0) return std::nullopt; //ping message
	if (header->instanceId < 1) return std::nullopt;
	auto body = std::string(data + 28, header->length - 28 - 1);
	protocol::GamePacketRequest packet(*header, body);
	std::optional<protocol::GamePacketRequest> modified = std::nullopt;
	for (auto& listener : sentListeners) {
		modified = listener(ws, packet);
	}
	return modified;
}

void Websocket::handleMessage(std::shared_ptr<Websocket> ws, const char* data, int size) {
	if (size < sizeof(protocol::Header)) return;
	int currentPos = 0;
	while (currentPos < size) {
		const auto* const header = reinterpret_cast<const protocol::Header*>(data + currentPos);
		if (header->length < 1 || currentPos + header->length > size) {
			break;
		}
		if (header->operationId == 0x80000001) {
			if (header->msgId == 0x1f0000 || header->msgId == 0x80000) {
				std::unique_ptr<char[]> body = std::make_unique<char[]>(size);
				std::memcpy(body.get(), data + currentPos + sizeof(protocol::Header), header->length);
				auto  b = protocol::StreamReader::fromBytes(std::move(body), header->length);
				automation::Lobby::instance().onLobbyPacket(ws, header->msgId, b);
			}
			currentPos += header->length + sizeof(protocol::Header);
		}
		else if (header->operationId == 0 && header->msgId == 0x63) {
			auto body = std::string(data + currentPos + sizeof(protocol::Header), header->length - sizeof(protocol::Header) - 1);
			protocol::GamePacket packet(*header, body);
			automation::Lobby::instance().onBudgetChange(packet);
			currentPos += header->length;
		}
		else {
			auto body = std::string_view(data + currentPos + sizeof(protocol::Header), header->length - sizeof(protocol::Header) - 1);
			currentPos += header->length;
			protocol::GamePacket packet(*header, body);
			for (auto& listener : recieveListeners) {
				listener(ws, packet);
			}
		}
	}
}

long long Websocket::sendMessage(std::shared_ptr<Websocket> ws, protocol::GamePacketRequest gp) {
	auto rawData = gp.prepare();
	return sendMessage(ws, rawData.data(), rawData.size());
}

long long Websocket::sendMessage(std::shared_ptr<Websocket> ws, char* data, size_t size) {
	std::lock_guard lock{ ws->mtx };
	QByteArray array = QByteArray(data, size);
	if (qtFunctions->socketState(ws->socket) == 3) {
		return qtFunctions->sendBinaryMessage(ws->socket, &array);
	}
	else {
		return 0;
	}
}

void Websocket::setInternalFunctions(std::shared_ptr<IInternalFunctions> functions) {
	qtFunctions = std::move(functions);
}
}
