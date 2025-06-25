#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace coinpoker {
namespace protocol {

struct BuyInRequest {
	std::string cid;
	std::string deposit_type_id;
	std::string game_event_type_id;
	std::string game_mod_id;
	std::string game_mode_name;
	std::string game_type_id;
	std::string instance_id;
	std::string name;
	std::string opid;
	std::string platform;
	std::string ssid;
	std::string table_id;
	std::string table_session_id;
	std::string totalAmount;

	std::string serialize() const {
		return nlohmann::json({
			{"cid", cid},
			{"deposit_type_id", deposit_type_id},
			{"game_event_type_id", game_event_type_id},
			{"game_mod_id", game_mod_id},
			{"game_mode_name", game_mode_name},
			{"game_type_id", game_type_id},
			{"instance_id", instance_id},
			{"name", name},
			{"opid", opid},
			{"platform", platform},
			{"ssid", ssid},
			{"table_id", table_id},
			{"table_session_id", table_session_id},
			{"totalAmount", totalAmount}
		}).dump();
	}
};

}
}
