#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <optional>
#include <string_view>

#include "model.h"
#include "app.h"



namespace requests {
    constexpr const char* GAME_TICK = "/api/v1/game/tick";
    constexpr const char* GAME_STATE = "/api/v1/game/state";
    constexpr const char* MAPS = "/api/v1/maps";
    constexpr const char* GAME_JOIN = "/api/v1/game/join";
    constexpr const char* GAME_PLAYERS = "/api/v1/game/players";
    constexpr const char* GAME_PLAYER_ACTION = "/api/v1/game/player/action";
    constexpr const char* MAPS_BY_ID = "/api/v1/maps/";
    constexpr const char* GAME_RECORDS = "/api/v1/game/records";
}

namespace response_errors {
    constexpr const char* BAD_REQUEST = "badRequest";
    constexpr const char* INVALID_METHOD = "invalidMethod";
    constexpr const char* MAP_NOT_FOUND = "mapNotFound";
    constexpr const char* INVALID_ARGUMENT = "invalidArgument";
}

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;



class ApiHandler {
public:
    struct ConfigScores {
        int start = 0;
        int max_items = 100;
    };

    using StringRequest = http::request<http::string_body>;
    using StringResponse = http::response<http::string_body>;

    explicit ApiHandler(app::Application& application);
    StringResponse HandleRequest(const StringRequest& req);

private:
    StringResponse HandleGetMaps();
    StringResponse HandleGetMapById(std::string_view map_id_str);
    StringResponse HandleJoinGame(const StringRequest& req);
    StringResponse HandleGetPlayers(const StringRequest& req);
    StringResponse HandleGameState(const StringRequest& req);
    StringResponse HandlePlayerSetAction(const StringRequest& req);
    StringResponse HandleGameTick(const StringRequest& req);
    StringResponse HandleGetRecords(const StringRequest& req);

    std::optional<app::Token> GetToken(const StringRequest& req) const;

    StringResponse MakeJsonResponse(http::status status, json::value&& data);
    StringResponse MakeErrorResponse(http::status status, std::string_view code, std::string_view message);
    StringResponse MakeMethodNotAllowed(std::string_view allowed_methods, std::string_view message = "Invalid method");

    template <typename Fn>
    StringResponse ExecuteAuthorized(const StringRequest& req, Fn&& action) {
        auto token = GetToken(req);

        if (!token) {
            return MakeErrorResponse(http::status::unauthorized, "invalidToken", "Authorization header is missing");
        }

        if (!application_.GetPlayers().FindPlayerByToken(*token)) {
            return MakeErrorResponse(http::status::unauthorized, "unknownToken", "Player token has not been found");
        }

        return action(*token);
    }

    ConfigScores GetConfigScoresFromUrl(std::string_view url) const;

    app::Application& application_;
};

}