#include "Client.hpp"
#include "Protocol.hpp"

#include <asio.hpp>
#include <string>

#include <array>

namespace coinpoker {
namespace aiapi {
// Can retry up to this many times on EOF on Client::StartGame before marking connection as terminated
static constexpr size_t kStartGameMaxNumberOfRetries{ 2 };

class ClientTcpTransport
{
private:
	static constexpr size_t kInboundBufferMaxSize{ 1024 };

	asio::io_context ioc;
	asio::ip::tcp::socket socket;
	asio::error_code ec;

public:
	friend Client;

	ClientTcpTransport() : ioc{}, socket{ ioc }
	{}

	void Connect(std::string_view ip, uint16_t port)
	{
		socket.connect({ asio::ip::make_address(ip.data()), port }, ec);
	}

	// Returns the last asio error that occured
	const asio::error_code& LastError() const noexcept { return ec; }

	// Send and receive a JSON string along the connection
	nlohmann::json SendReceiveJson(const nlohmann::json& data)
	{
		if (!socket.is_open()) {
			return nullptr;
		}

		const std::string rawData = data.dump();

		asio::write(socket, asio::buffer(rawData), asio::transfer_all(), ec);
		if (ec) {
			return nullptr;
		}

		std::array<char, kInboundBufferMaxSize> buf{};
		asio::read(socket, asio::buffer(buf), asio::transfer_at_least(1), ec);

		if (ec) {
			return nullptr;
		}

		return nlohmann::json::parse(buf.data());
	}
};

Client::Client(std::string_view ip, uint16_t port)
	: mLogger{ utils::CreateLogger("aiapi") }
	, mIp{ ip.data() }
	, mPort{ port }
	, mTransport{ std::make_unique<ClientTcpTransport>() }
{
	if (mTransport->Connect(ip, port); mTransport->LastError()) {
		mLogger->error("Could not connect, error: {}", LastError());
	}
}
Client::~Client() = default;

std::string Client::LastError() const
{
	return mTransport->LastError().message();
}
std::optional<Response> Client::Perform(const nlohmann::json& json)
{
	try
	{
		mLogger->debug("Sending: {}", json.dump());

		const nlohmann::json j = mTransport->SendReceiveJson(json);
		if (j.is_null())
		{
			mLogger->error("Could not send, error: {}", LastError());
			return std::nullopt;
		}

		mLogger->debug("Received: {}", j.dump());

		auto response = std::make_optional<Response>(j);
		if (!response->success)
		{
			mLogger->error("Processing error: {}", response->error_msg);
			return std::nullopt;
		}

		return response;
	}
	catch (std::exception& e)
	{
		mLogger->error("Perform() exception: {}", e.what());
		return std::nullopt;
	}
}

void Client::StartGame(
	std::string_view platform_name,
	uint64_t sb,
	uint64_t bb,
	uint64_t ante,
	uint64_t straddle,
	uint64_t cashDrop,
	std::string_view heroPlayerId,
	const PlayerGetter<PlayerInfo>& getPlayer,
	const std::vector<std::string>& postedExtraPlayerIds
)
{
	RequestStartGame request
	{
		.message_type = MessageType_StartGame,
		.game_type = "cash",
		.platform_name = platform_name.data(),
		.n_players = 0,
		.sb = sb,
		.bb = bb,
		.ante = ante,
		.straddle = straddle,
		.cashdrop = cashDrop,
		.ai_player_pid = heroPlayerId.data(),
		.post_a_blind_pids = postedExtraPlayerIds,
		.player_info = {},
	};

	for (;; ++request.n_players)
	{
		auto info = getPlayer();

		if (!info)
		{
			// No more players to enumerate
			break;
		}

		request.player_info.emplace_back(std::move(*info));
	}

	for (size_t retryCount = 0;

		retryCount < kStartGameMaxNumberOfRetries
		&& !Perform(request)
		&& LastError() == "End of file";

		retryCount++)
	{
		mLogger->info("Attempting reconnection...");

		mTransport = std::make_unique<ClientTcpTransport>();

		if (mTransport->Connect(mIp, mPort); mTransport->LastError())
		{
			mLogger->error("Could not connect, error: {}", LastError());
			return;
		}
	}
}

std::optional<ActionSuggestion> Client::PrivateHand(
	const std::vector<std::string>& cards
)
{
	RequestPrivateHand request
	{
		.message_type = MessageType_PrivateHand,
		.private_hand = cards
	};

	const auto response = Perform(request);
	if (!response || response->ai_action.bet_type == ActionType::ActionType_Invalid) {
		return std::nullopt;
	}

	return ActionSuggestion{
		response->ai_action.bet_type,
		response->ai_action.amount
	};
}

std::optional<ActionSuggestion> Client::DealCommunityCards(
	const std::vector<std::string>& cards
)
{
	RequestDealCommunityCards request
	{
		.message_type = MessageType_DealCommunityCards,
		.deal_community_cards = cards
	};

	const auto response = Perform(request);
	if (!response || response->ai_action.bet_type == ActionType::ActionType_Invalid) {
		return std::nullopt;
	}

	return ActionSuggestion{
		response->ai_action.bet_type,
		response->ai_action.amount
	};
}


std::optional<ActionSuggestion> Client::PlayerMove(
	std::string_view playerId,
	ActionType type,
	uint64_t amount
)
{
	RequestPlayerMove request
	{
		.message_type = MessageType_PlayerMove,
		.action = ActionInfo
		{
			.player_pid = playerId.data(),
			.bet_type = type,
			.amount = amount,
		}
	};

	const auto response = Perform(request);
	if (!response || response->ai_action.bet_type == ActionType::ActionType_Invalid) {
		return std::nullopt;
	}

	return ActionSuggestion{
		response->ai_action.bet_type,
		response->ai_action.amount
	};
}

void Client::Showdown(
	const PlayerGetter<ShowdownInfo>& getPlayer
)
{
	RequestShowdown request
	{
		.message_type = MessageType_Showdown,
		.showdown_info = {},
	};

	while (true)
	{
		auto info = getPlayer();

		if (!info)
		{
			// No more players to enumerate
			break;
		}

		request.showdown_info.emplace_back(std::move(*info));
	}

	Perform(request);
}
}  // namespace aiapi
}
