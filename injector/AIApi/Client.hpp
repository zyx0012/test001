#pragma once

#include "../utils/logging.hpp"
#include "Protocol.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <string_view>
#include <functional>
#include <string>
#include <optional>
#include <functional>

namespace coinpoker {
namespace aiapi {
//
// Private API
//

class ClientTcpTransport;
struct Response;

//
// Public API
//

template<class T>
using PlayerGetter = std::function<std::optional<T>()>;

struct ActionSuggestion
{
	ActionType type{};
	float amount{};
};

class Client
{
private:
	utils::Logger mLogger{};

	std::string mIp{};
	uint16_t mPort{};

	// Internal transport implementation
	std::unique_ptr<ClientTcpTransport> mTransport{};

	// Returns the last error message from transport
	// Undefined behaviour if there is no error
	std::string LastError() const;

	// Executes a JSON defined request to AI API
	std::optional<Response> Perform(const nlohmann::json& json);

public:
	Client(std::string_view ip, uint16_t port);
	~Client();

	//
	// Syntactic sugar for each message type
	//

	void StartGame(
		std::string_view platform_name,
		uint64_t sb,
		uint64_t bb,
		uint64_t ante,
		uint64_t straddle,
		uint64_t cashDrop,
		std::string_view heroPlayerId,
		const PlayerGetter<PlayerInfo>& getPlayer,
		const std::vector<std::string>& postedExtraPlayerIds
	);

	std::optional<ActionSuggestion> PrivateHand(
		const std::vector<std::string>& cards
	);

	std::optional<ActionSuggestion> DealCommunityCards(
		const std::vector<std::string>& cards
	);

	std::optional<ActionSuggestion> PlayerMove(
		std::string_view playerId,
		ActionType type,
		uint64_t amount
	);

	void Showdown(
		const PlayerGetter<ShowdownInfo>& getPlayer
	);
};
}  // namespace aiapi
}
