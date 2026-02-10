#pragma once

#include "api_handler.h"
#include "app.h"
#include "http_server.h"
#include "model.h"

#include <boost/json.hpp>
#include <unordered_map>
#include <variant>
#include <filesystem>


namespace http_handler {

static const std::string UNKNOWN_MIME = "application/octet-stream";

static const std::unordered_map<std::string, std::string> MIME_TYPES = {
    {".htm", "text/html"},
    {".html", "text/html"},
    {".css", "text/css"},
    {".txt", "text/plain"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpe", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".bmp", "image/bmp"},
    {".ico", "image/vnd.microsoft.icon"},
    {".tiff", "image/tiff"},
    {".tif", "image/tiff"},
    {".svg", "image/svg+xml"},
    {".svgz", "image/svg+xml"},
    {".mp3", "audio/mpeg"}
};

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace fs = std::filesystem;
namespace net = boost::asio;

class RequestHandler {
    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;
    // под файлы
    using FileResponse = http::response<http::file_body>;
    // Variant для поддержки обоих типов ответов
    using ResponseVariant = std::variant<StringResponse, FileResponse>;

public:
    explicit RequestHandler(model::Game& game, std::filesystem::path static_path, 
        app::Application& application, net::strand<net::io_context::executor_type> api_strand);

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        auto target = std::string(req.target());

        if(target.find("/api/") == 0) {           
            net::dispatch(api_strand_, 
                [this, req, send] () {
                auto response = api_handler_.HandleRequest(req);
                send(std::move(response));
            });
        }
        else {
            auto response = HandleRequestFile(req);
            std::visit([&send](auto&& resp) { send(std::move(resp)); }, response);
        }
    }

private:

    StringResponse MapsResponseJSON();
    StringResponse MapIdInfoResponseJSON(const model::Map* map);
    StringResponse ErrorResponseJSON(http::status status);

    ResponseVariant HandleRequestFile(const StringRequest& req);

    StringResponse ErrorResponseFile(http::status status, std::string_view content_type, std::string_view body);

    std::string DecodeURI(std::string_view encoded_str);
    std::string GetMimeType(const fs::path& file_path);
    bool IsSubPath(fs::path path, fs::path base);

    ApiHandler api_handler_;
    model::Game& game_;
    std::filesystem::path static_path_;
    net::strand<net::io_context::executor_type> api_strand_;
};

};  // namespace http_handler