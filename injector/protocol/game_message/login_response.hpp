#pragma once

#include <string>
#include "../stream_reader.hpp"
#include <nlohmann/json.hpp>

namespace coinpoker {
namespace protocol {

class LoginReponse {
public:
	uint32_t userId;
	std::string userName{};
	std::string nickName{};
	std::string profileImage{};
	std::string email{};
	uint32_t usdtBalance{};
	uint32_t chpBalance{};
	std::string ssid{};

	static LoginReponse parse(StreamReader& reader) {
		LoginReponse res;
		reader.skip(8);
		res.userId = reader.readUInt32();
		res.userName = reader.readString();
		res.nickName = reader.readString();
		res.profileImage = reader.readString();
		res.email = reader.readString();
		reader.readUInt32();
		reader.readUInt32();
		reader.readUInt32();
		reader.readUInt8();
		res.usdtBalance = reader.readUInt32();
		reader.skip(12);
		reader.readString();
		res.chpBalance = reader.readUInt32();
		reader.skip(12);
		reader.readString();
		res.ssid = reader.readString();
		return res;
	}

	nlohmann::json toJson() const {
		return {
			{ "userId", userId },
			{ "userName", userName },
			{ "nickName", nickName },
			{ "profileImage", profileImage },
			{ "email", email },
			{ "usdtBilance", usdtBalance },
			{ "chpBilance", chpBalance },
			{ "ssid", ssid }
		};
	}
};


}
}
