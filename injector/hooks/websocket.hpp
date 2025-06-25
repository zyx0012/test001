#pragma once

#include "hook.hpp"
#include "../utils/utils.hpp"
#include "internal_functions.hpp"
#include <functional>
#include <vector>
#include <mutex>
#include <unordered_map>
#include "../protocol/game_packet.hpp"
#include <optional>

namespace coinpoker {


class Websocket {

using Listener = std::function<void(std::shared_ptr<Websocket> ws, protocol::GamePacket&)>;
using SendListener = std::function<std::optional<protocol::GamePacketRequest>(std::shared_ptr<Websocket> ws, protocol::GamePacketRequest&)>;

public:

	Websocket(QWebSocketPtr s) : socket(s) {}

	std::mutex mtx{};

	static std::unordered_map<QWebSocketPtr, std::shared_ptr<Websocket>> sockets;

	static std::shared_ptr<Websocket> getSocket(QWebSocketPtr webSocket);
	static int onMessageRecieve(QWebSocketPtr webSocket, QByteArray* buffer);
	static long long onMessageSent(QWebSocketPtr webSocket, QByteArray* buffer);
	static void addRecieveListener(const Listener& listener);
	static void addSentListener(const SendListener& listener);
	static void setInternalFunctions(std::shared_ptr<IInternalFunctions> qtFunctions);
	static long long sendMessage(std::shared_ptr<Websocket>, protocol::GamePacketRequest);
	static long long sendMessage(std::shared_ptr<Websocket>, char* data, size_t size);
	QWebSocketPtr socket;

private:
	static void handleMessage(std::shared_ptr<Websocket> ws, const char* buffer, int size);
	static std::optional<protocol::GamePacketRequest> handleSentMessage(std::shared_ptr<Websocket> ws,const char* buffer, int size);
	static std::vector<Listener> recieveListeners;
	static std::vector<SendListener> sentListeners;
	static std::shared_ptr<IInternalFunctions> qtFunctions;
};

}