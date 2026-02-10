#include "api_handler.h"

#include <algorithm> 
#include <cctype>
#include <boost/algorithm/string/predicate.hpp>

namespace http_handler {

using namespace json_fields;
using namespace response_errors;

    ApiHandler::ApiHandler(app::Application& application): application_(application) {} 

    ApiHandler::StringResponse ApiHandler::HandleRequest(const StringRequest& req) {
        const auto target = std::string(req.target());
        const auto method = req.method();

        if (target == requests::GAME_TICK) {
            if (method == http::verb::post) {
                return HandleGameTick(req);
            }
            return MakeMethodNotAllowed("POST");
        }


        if (target == requests::GAME_STATE) {
            if (method == http::verb::get || method == http::verb::head) {
                return HandleGameState(req);
            }
            return MakeMethodNotAllowed("GET, HEAD");
        }

        if (target == requests::GAME_JOIN) {
            if (method == http::verb::post) {
                return HandleJoinGame(req);
            }
            return MakeMethodNotAllowed("POST");
        }
        
        if (target == requests::GAME_PLAYERS) {
            if (method == http::verb::get || method == http::verb::head) {
                return HandleGetPlayers(req);
            }
            return MakeMethodNotAllowed("GET, HEAD");
        }

        if (target == requests::GAME_PLAYER_ACTION) {
            if (method == http::verb::post) {
                return HandlePlayerSetAction(req);
            }
            return MakeMethodNotAllowed("POST");
        }

        if (target == requests::MAPS) {
            if (method == http::verb::get) {
                return HandleGetMaps();
            }
            return MakeMethodNotAllowed("GET");
        }

        if (target.find(requests::GAME_RECORDS) == 0) {
            if (method == http::verb::get || method == http::verb::head) {
                return HandleGetRecords(req);
            }
            return MakeMethodNotAllowed("GET, HEAD");
        }

        if (target.find(requests::MAPS_BY_ID) == 0) {
            if (method == http::verb::get || method == http::verb::head) {
                std::string map_id = target.substr(std::string(requests::MAPS_BY_ID).length());
                return HandleGetMapById(map_id);
            }
            return MakeMethodNotAllowed("GET, HEAD");
        }

        return MakeErrorResponse(http::status::bad_request, BAD_REQUEST, "Bad request");
    }

    ApiHandler::StringResponse ApiHandler::MakeJsonResponse(http::status status, json::value&& data) {
        StringResponse response;
        response.result(status);
        response.set(http::field::content_type, "application/json");
        response.set(http::field::cache_control, "no-cache");
        response.body() = json::serialize(data);
        response.content_length(response.body().size());
        return response;
    }

    ApiHandler::StringResponse ApiHandler::MakeErrorResponse(http::status status, std::string_view code, std::string_view message) {
        json::value error_response = json::object{
            {"code", code},
            {"message", message}
        };
        return MakeJsonResponse(status, std::move(error_response));
    }

    ApiHandler::StringResponse ApiHandler::MakeMethodNotAllowed(std::string_view allowed_methods, std::string_view message) {
        auto response = MakeErrorResponse(http::status::method_not_allowed, INVALID_METHOD, message);
        response.set(http::field::allow, allowed_methods);
        return response;
    }

    ApiHandler::StringResponse ApiHandler::HandleGetMaps() {
        json::array maps_array;

        for (const auto& map : application_.ListMaps()) {
            maps_array.push_back(json::object{
                { map_fields::ID, *map.GetId() },
                { map_fields::NAME, map.GetName() }
            });
        }
        
        return MakeJsonResponse(http::status::ok, std::move(maps_array));
    }

    ApiHandler::StringResponse ApiHandler::HandleGetMapById(std::string_view map_id_str) {

        const auto* map = application_.FindMap(map_id_str);

        if (map == nullptr) {
            return MakeErrorResponse(http::status::not_found, MAP_NOT_FOUND, "Map not found");
        }

        json::value map_result = json::object{
            { map_fields::ID, *map->GetId() },
            { map_fields::NAME, map->GetName() }
        };

        json::array roads_array;
        for (const auto& road : map->GetRoads()) {
            
            auto start = road.GetStart();
            auto end = road.GetEnd();

            if(road.IsHorizontal()) {
                roads_array.push_back(json::object{
                    { road_fields::X0, start.x },
                    { road_fields::Y0, start.y },
                    { road_fields::X1, end.x }
                });
            }
            else {
                roads_array.push_back(json::object{
                    { road_fields::X0, start.x },
                    { road_fields::Y0, start.y },
                    { road_fields::Y1, end.y }
                });
            }
        }

        json::array buildings_array;
        for (const auto& building : map->GetBuildings()) {
            auto bounds = building.GetBounds();
            buildings_array.push_back(json::object{
                { building_fields::X, bounds.position.x },
                { building_fields::Y, bounds.position.y },
                { building_fields::WIDTH, bounds.size.width },
                { building_fields::HEIGHT, bounds.size.height }
            });
        }

        json::array offices_array;
        for (const auto& office : map->GetOffices()) {
            auto pozition = office.GetPosition();
            auto offset = office.GetOffset();
            offices_array.push_back(json::object{
                { office_fields::ID, *office.GetId() },
                { office_fields::X, pozition.x },
                { office_fields::Y, pozition.y },
                { office_fields::OFFSET_X, offset.dx },
                { office_fields::OFFSET_Y, offset.dy }
            });
        }

        auto extra_data = map->GetExtraData();
        auto loot_types = extra_data.GetLootTypes();

        map_result.as_object()[map_fields::ROADS] = std::move(roads_array);
        map_result.as_object()[map_fields::BUILDINGS] = std::move(buildings_array);
        map_result.as_object()[map_fields::OFFICES] = std::move(offices_array);
        map_result.as_object()[map_fields::LOOT_TYPES] = std::move(loot_types);
        map_result.as_object()[map_fields::SPEED] = map->GetDogSpeed();
        map_result.as_object()[map_fields::BAG_CAPACITY] = map->GetBagCapacity();

        return MakeJsonResponse(http::status::ok, std::move(map_result));
    }

    ApiHandler::StringResponse ApiHandler::HandleJoinGame(const StringRequest& req) {
        if (req.body().empty()) {
            return MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Join game request body is empty");
        }

        json::value req_body;
        try {
            req_body = json::parse(req.body());
        }
        catch (const std::exception& e) {
            return MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Join game request parse error");
        }

        std::string user_name;
        std::string map_id_str;


        auto& obj = req_body.as_object();

        if (obj.contains("userName") && !obj.at("userName").get_string().empty()) {
            user_name = obj.at("userName").get_string();
        }
        else {
            return MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Invalid mapId");
        }

        if (obj.contains("mapId") && !obj.at("mapId").get_string().empty()) {
            map_id_str = obj.at("mapId").get_string();
        }
        else {
            return MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Invalid mapId");
        }

        try {
            auto [token, player_ptr] = application_.JoinGame(map_id_str, user_name);

            json::value result = json::object{
                {"authToken", *token},
                {"playerId", *player_ptr }
            };

            return MakeJsonResponse(http::status::ok, std::move(result));
        }
        catch (const std::exception& e) {
            return MakeErrorResponse(http::status::not_found, MAP_NOT_FOUND, "Map not found");
        }

    }

    std::optional<app::Token> ApiHandler::GetToken(const StringRequest& req) const {
        auto it = req.find(http::field::authorization);
        
        if (it != req.end()) {
            std::string_view header_value = it->value();
            constexpr std::string_view BEARER_PREFIX = "Bearer "; 

            if (boost::algorithm::starts_with(header_value, BEARER_PREFIX)) {
                std::string_view token_str_view = header_value.substr(BEARER_PREFIX.length());
                
                if (!token_str_view.empty() && token_str_view.length() == 32 && 
                    std::all_of(token_str_view.begin(), token_str_view.end(), 
                    [](char c){ return std::isxdigit(c); })) {
                    return app::Token{std::string(token_str_view)}; 
                }
                
            }
        }
        return std::nullopt;
    }

    ApiHandler::StringResponse ApiHandler::HandleGetPlayers(const StringRequest& req) {
        return ExecuteAuthorized(req, [this](const app::Token& token) {
            std::shared_ptr<app::Player> player = application_.GetPlayers().FindPlayerByToken(token);
            auto session_dogs = player->GetSession()->GetDogs();
            json::object players_list;

            for (const auto& [dog_id, dog] : session_dogs) {
                json::value dog_info = json::object{
                    {"name", dog->GetName()}
                };
                
                players_list[std::to_string(*dog_id)] = std::move(dog_info);
            }
            
            json::object result_data;
            result_data["players"] = std::move(players_list);

            return this->MakeJsonResponse(http::status::ok, std::move(result_data));
        });
    }

    ApiHandler::StringResponse ApiHandler::HandleGameState(const StringRequest& req) {
        return ExecuteAuthorized(req, [this](const app::Token& token) {
            
            auto state = application_.GameState(token);

            json::object game_state_players;

            for (const auto& [id, player] : state.dogs) {
                auto position = player->GetPosition();
                auto speed = player->GetSpeed();
                auto direction = model::DirectionToString(player->GetDirection());
                json::array bag;
                size_t score = player->GetScore();

                for (const auto& item : player->GetItemsFromBag()) {
                    bag.push_back(json::object{
                        { "id", *item.id },
                        { "type", item.type }
                    });
                }

                json::value player_info = json::object {
                    { "pos", json::array{position.x, position.y} },
                    { "speed", json::array{speed.x, speed.y} },
                    { "dir", direction },
                    { "bag", bag },
                    { "score", score }
                };

                game_state_players[std::to_string(*id)] = std::move(player_info);
            }

            json::object game_state_types;

            for (const auto& [id, loot] : state.loots) {
                auto type = loot->GetType();
                auto position = loot->GetPosition();

                json::value loot_info = json::object {
                    { "type", type },
                    { "pos", json::array{position.x, position.y} }
                };

                game_state_types[std::to_string(*id)] = std::move(loot_info);
            }

            json::object result_data;
            result_data["players"] = std::move(game_state_players);
            result_data["lostObjects"] = std::move(game_state_types);

            return this->MakeJsonResponse(http::status::ok, std::move(result_data));
        });
    }

    ApiHandler::StringResponse ApiHandler::HandlePlayerSetAction(const StringRequest& req) {
        return ExecuteAuthorized(req, [this, req](const app::Token& token) {
            if (req[http::field::content_type] != "application/json") {
                return this->MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Invalid content type");
            }

            json::value req_body;
            try {
                req_body = json::parse(req.body());
            }
            catch(const std::exception& e) {
                return this->MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Failed to parse action");
            }

            if (!req_body.is_object() || !req_body.as_object().contains("move")) {
                return this->MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Failed to parse action");
            }

            if (!req_body.as_object().at("move").is_string()) {
                return this->MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Failed to parse action");
            }

            std::string_view move_direction = req_body.as_object().at("move").get_string();

            if (move_direction != move_direction::UP && 
                move_direction != move_direction::DOWN && 
                move_direction != move_direction::LEFT && 
                move_direction != move_direction::RIGHT &&
                move_direction != move_direction::STOP) {
                return this->MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Failed to parse action");
            }

            application_.SetPlayerAction(token, move_direction);
            json::value result = json::object{};
            return this->MakeJsonResponse(http::status::ok, std::move(result));
        });
    }

    ApiHandler::StringResponse ApiHandler::HandleGameTick(const StringRequest& req) {
        if (application_.IsAutoTickEnabled()) {
            return MakeErrorResponse(http::status::bad_request, BAD_REQUEST, "Invalid endpoint");
        }

        if (req.body().empty()) {
            return MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Join game request body is empty");
        }

        json::value req_body;
        try {
            req_body = json::parse(req.body());
        }
        catch(const std::exception& e) {
            return MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Failed to parse action");
        }

        if (!req_body.is_object() || !req_body.as_object().contains("timeDelta")) {
            return MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Failed to parse action");
        }

        if (!req_body.as_object().at("timeDelta").is_int64()) {
            return MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Failed to parse action");
        }

        int64_t time_ms = req_body.as_object().at("timeDelta").get_int64();

        if (time_ms <= 0) {
            return MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Failed time");
        }

        std::chrono::milliseconds time_delta(time_ms);
        application_.Tick(time_delta);

        json::value result = json::object{};
        return MakeJsonResponse(http::status::ok, std::move(result));
    }

    ApiHandler::ConfigScores ApiHandler::GetConfigScoresFromUrl(std::string_view url) const {
        ConfigScores config;

        auto pos_beg = url.find('?');
        if (pos_beg == std::string_view::npos || pos_beg + 1 >= url.length()) {
            return config;
        }

        std::string start_str = "start=";
        std::string items_str = "maxItems=";

        std::string_view query = url.substr(pos_beg + 1);
       
        auto parse_str = [](std::string_view str, std::string item_name, size_t pos_start) {
            auto pos_start_full = pos_start + item_name.length();
            auto end_pos = str.find_first_not_of("-0123456789", pos_start_full);

            if (end_pos == std::string::npos) {
                return std::stoi(std::string(str.substr(pos_start_full)));
            }
            else {
                return std::stoi(std::string(str.substr(pos_start_full, end_pos - pos_start_full)));
            }
        }; 
        
        auto pos_start = query.find(start_str);

        if (pos_start != std::string_view::npos) {
            config.start = parse_str(query, start_str, pos_start);
        }

        auto pos_max_items = query.find("maxItems=");

        if (pos_max_items != std::string_view::npos) {
            config.max_items = parse_str(query, items_str, pos_max_items);
        }

        return config;
    }

    ApiHandler::StringResponse ApiHandler::HandleGetRecords(const StringRequest& req) {
        ConfigScores config;

        try {
            config = GetConfigScoresFromUrl(req.target());
        }
        catch (const std::exception& e) {
            return MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Invalid argument: start and maxItems must be valid integers");
        }

        if (config.start < 0 || config.max_items < 0 || config.max_items > 100) {
            return MakeErrorResponse(http::status::bad_request, INVALID_ARGUMENT, "Failed to parse config");
        }

        auto records = application_.Records(config.start, config.max_items);

        json::array result;

        for (const auto& player : records) {
            auto time = static_cast<double>(player.GetTimeMs());
            time /= 1000.0;
            result.push_back(json::object{
                { "name", player.GetName() },
                { "score", player.GetScore() },
                { "playTime", time }
            });
        }

        return MakeJsonResponse(http::status::ok, std::move(result));
    }
}