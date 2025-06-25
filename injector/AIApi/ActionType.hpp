#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace coinpoker {
namespace aiapi {
enum ActionType
{
	ActionType_Invalid = -1,
	ActionType_Fold,
	ActionType_Call,
	ActionType_Check,
	ActionType_Raise,
	ActionType_Bet,
	ActionType_Allin
};

NLOHMANN_JSON_SERIALIZE_ENUM(ActionType, {
	{ActionType_Invalid, nullptr},
	{ActionType_Fold, "folds"},
	{ActionType_Call, "calls"},
	{ActionType_Check, "checks"},
	{ActionType_Raise, "raises"},
	{ActionType_Bet, "bets"},
	{ActionType_Allin, "allins"},
	})
}  // namespace aiapi
}
