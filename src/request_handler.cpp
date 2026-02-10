
#include "request_handler.h"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <cctype>

namespace http_handler {

    using namespace json_fields;

    RequestHandler::RequestHandler(model::Game& game, std::filesystem::path static_path, 
        app::Application& application, net::strand<net::io_context::executor_type> api_strand)
        : game_{game}, static_path_(static_path), 
        api_handler_(application),
        api_strand_(api_strand) {}

    RequestHandler::StringResponse RequestHandler::MapsResponseJSON() {
            json::array maps_array;

            for (const auto& map : game_.GetMaps()) {
                maps_array.push_back(json::object{
                    { map_fields::ID, *map.GetId() },
                    { map_fields::NAME, map.GetName() }
                });
            }
            
            StringResponse response;
            response.result(http::status::ok);
            response.set(http::field::content_type, "application/json");
            response.body() = json::serialize(maps_array);
            response.content_length(response.body().size());
            return response;
    }

    RequestHandler::StringResponse RequestHandler::ErrorResponseJSON(http::status status) {
            StringResponse response;
            response.result(status);
            response.set(http::field::content_type, "application/json");

            if(status == http::status::bad_request) {
                json::value  error_response = json::object{
                    { "code", "badRequest" },
                    { "message", "Bad request" }
                };
                response.body() = json::serialize(error_response);
            }
            else if (status == http::status::not_found) {
                json::value  error_response = json::object{
                    { "code", "mapNotFound" },
                    { "name", "Map not found" }
                };
                response.body() = json::serialize(error_response);
            }

            response.content_length(response.body().size());
            return response;
    }

    RequestHandler::StringResponse RequestHandler::MapIdInfoResponseJSON(const model::Map* map) {
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

            map_result.as_object()[map_fields::ROADS] = std::move(roads_array);
            map_result.as_object()[map_fields::BUILDINGS] = std::move(buildings_array);
            map_result.as_object()[map_fields::OFFICES] = std::move(offices_array);

            StringResponse response;
            response.result(http::status::ok);
            response.set(http::field::content_type, "application/json");
            response.body() = json::serialize(map_result);
            response.content_length(response.body().size());
            return response;
    }

    RequestHandler::StringResponse RequestHandler::ErrorResponseFile(http::status status, std::string_view content_type, std::string_view body) {
            StringResponse response;
            response.result(status);
            response.set(http::field::content_type, content_type);
            response.body() = body;
            response.prepare_payload();
            return response;
    }
    
    RequestHandler::ResponseVariant RequestHandler::HandleRequestFile(const StringRequest& req) {

        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            return ErrorResponseFile(http::status::method_not_allowed, "text/plain", "Invalid method");
        }

        // декодируем URR
        std::string target_path;

        try {
            target_path = DecodeURI(req.target());
        }
        catch(const std::invalid_argument& e) {
            return ErrorResponseFile(http::status::bad_request, "text/plain", "Invalid path");
        }


        // если пустой путь или корневой
        if (target_path == "/" || target_path.empty()) {
            target_path = "index.html";
        }

        // минус слэш
        if (target_path[0] == '/') {
            target_path = target_path.substr(1);
        }

        fs::path fail_path = static_path_ / target_path;

        // проверка, что путь внутри корневой директории
        if (!IsSubPath(fail_path, static_path_)) {
            return ErrorResponseFile(http::status::bad_request, "text/plain", "Invalid path");
        }

        // если директория - искать index.html внутри
        if (fs::is_directory(fail_path)) {
            fail_path /= "index.html";
        }

        // проверка на существование файла
        if (!fs::exists(fail_path) || !fs::is_regular_file(fail_path)) {
            return ErrorResponseFile(http::status::not_found, "text/plain", "File not found");
        }

        // Создаем ответ с файлом
        FileResponse response;
        response.version(req.version());
        response.result(http::status::ok);
        response.set(http::field::content_type, GetMimeType(fail_path));
        response.set(http::field::cache_control, "no-cache");

        // Открываем файл
        beast::error_code ec;
        http::file_body::value_type file;
        file.open(fail_path.c_str(), beast::file_mode::read, ec);
        
        if (ec) {
            return ErrorResponseFile(http::status::not_found, "text/plain", "File not found");
        }

        response.body() = std::move(file);
        response.prepare_payload();
        
        return response;
    }

    std::string RequestHandler::DecodeURI(std::string_view encoded_str) {
        std::string result;

        for (int i(0); i < encoded_str.size(); ++i) {
            if (encoded_str[i] == '+') {
                result += ' ';
            }
            else if (encoded_str[i] == '%') {

                // Проверяем, что есть хотя бы 2 символа после %
                if (i + 2 >= encoded_str.size()) {
                    throw std::invalid_argument("Incomplete % sequence");
                }
                
                // Проверяем, что следующие 2 символа - валидные hex-цифры
                char hex1 = encoded_str[i + 1];
                char hex2 = encoded_str[i + 2];
                
                if (!isxdigit(hex1) || !isxdigit(hex2)) {
                    throw std::invalid_argument("Invalid hex digits in % sequence");
                }

                int value = 0;
                std::istringstream hex_value(std::string(encoded_str.substr(i + 1, 2)));
                if(hex_value >> std::hex >> value) {
                    result += static_cast<char>(value);
                    i += 2;
                }
                else {
                    result += encoded_str[i];
                }
            }
            else {
                result += encoded_str[i];
            }
        }

        return result;
    }

    std::string RequestHandler::GetMimeType(const fs::path& file_path) {

        std::string extension = file_path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        auto it = MIME_TYPES.find(extension);
        if(it != MIME_TYPES.end()) {
            return it->second;
        }

        return UNKNOWN_MIME;
    }

    // Возвращает true, если каталог p содержится внутри base_path.
    bool RequestHandler::IsSubPath(fs::path path, fs::path base) {
        // Приводим оба пути к каноничному виду (без . и ..)
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        // Проверяем, что все компоненты base содержатся внутри path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }

}  // namespace http_handler