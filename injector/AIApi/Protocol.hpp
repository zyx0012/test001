#pragma once

#include "ActionType.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace coinpoker {
namespace aiapi {
enum MessageType
{
	MessageType_Invalid = -1,
	MessageType_StartGame,
	MessageType_PrivateHand,
	MessageType_DealCommunityCards,
	MessageType_PlayerMove,
	MessageType_Showdown,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MessageType, {
	{MessageType_Invalid, nullptr},
	{MessageType_StartGame, "start_game"},
	{MessageType_PrivateHand, "private_hand"},
	{MessageType_DealCommunityCards, "deal_community_cards"},
	{MessageType_PlayerMove, "player_move"},
	{MessageType_Showdown, "showdown"},
	})

	// Generic response structure returned by AI API.
	struct AIActionInfo
{
	ActionType bet_type{ ActionType_Invalid };
	float amount{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AIActionInfo
	, bet_type
	, amount
)

struct Response
{
	bool success{};
	std::string error_msg{};
	AIActionInfo ai_action{};
};

inline void to_json(nlohmann::json& j, const Response& r)
{
	j = nlohmann::json{ {"success", r.success}, {"error_msg", r.error_msg}, {"ai_action", r.ai_action} };
}
inline void from_json(const nlohmann::json& j, Response& r)
{
	j.at("success").get_to(r.success);
	j.at("error_msg").get_to(r.error_msg);
	if (j.contains("ai_action"))
	{
		j.at("ai_action").get_to(r.ai_action);
	}
}

//
// Injection to AI API (Outbound) types
//

struct PlayerInfo
{
	std::string pid{};
	uint64_t stack{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PlayerInfo
	, pid
	, stack
)

struct ActionInfo
{
	std::string player_pid{};
	ActionType bet_type{};
	uint64_t amount{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ActionInfo
	, player_pid
	, bet_type
	, amount
)

struct ShowdownInfo
{
	std::string pid{};
	int64_t payoff{};
	int64_t rake{};
	std::vector<std::string> showdown_cards{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShowdownInfo
	, pid
	, payoff
	, rake
	, showdown_cards
)

struct RequestStartGame
{
	MessageType message_type{};
	std::string game_type{}; // "cash"
	std::string platform_name{};
	uint32_t n_players{};
	uint64_t sb{};
	uint64_t bb{};
	uint64_t ante{};
	uint64_t straddle{};
	uint64_t cashdrop{};
	std::string ai_player_pid{};
	std::vector<std::string> post_a_blind_pids{};
	std::vector<PlayerInfo> player_info{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RequestStartGame
	, message_type
	, game_type
	, platform_name
	, n_players
	, sb
	, bb
	, ante
	, straddle
	, cashdrop
	, ai_player_pid
	, post_a_blind_pids
	, player_info
)

struct RequestPrivateHand
{
	MessageType message_type{};
	std::vector<std::string> private_hand{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RequestPrivateHand
	, message_type
	, private_hand
)

struct RequestDealCommunityCards
{
	MessageType message_type{};
	std::vector<std::string> deal_community_cards{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RequestDealCommunityCards
	, message_type
	, deal_community_cards
)

struct RequestPlayerMove
{
	MessageType message_type{};
	ActionInfo action{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RequestPlayerMove
	, message_type
	, action
)

struct RequestShowdown
{
	MessageType message_type{};
	std::vector<ShowdownInfo> showdown_info{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RequestShowdown
	, message_type
	, showdown_info
)
}  // namespace aiapi
}
