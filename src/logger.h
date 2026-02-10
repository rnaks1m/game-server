#pragma once

#include <chrono>
#include <utility>
#include <variant>
#include <string>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp> 
#include <boost/log/attributes.hpp>
#include <boost/date_time.hpp>

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/console.hpp>

#include "request_handler.h"

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace net = boost::asio;
namespace sys = boost::system;

BOOST_LOG_ATTRIBUTE_KEYWORD(data_attr, "Data", json::value);
BOOST_LOG_ATTRIBUTE_KEYWORD(message_attr, "Message", std::string);
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime);


void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

void InitCustomConsoleLog();

namespace logger {

void LogServerStart(unsigned short port, net::ip::address address);
void LogServerStop();
void LogServerStopEx(const std::exception& ex, int code);
void LogServerError(const sys::error_code& ec, std::string where);

template<class SomeRequestHandler>
class LoggingRequestHandler {
    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;
    // под файлы
    using FileResponse = http::response<http::file_body>;
    // Variant для поддержки обоих типов ответов
    using ResponseVariant = std::variant<StringResponse, FileResponse>;

public:

    LoggingRequestHandler(SomeRequestHandler& decorated, const net::ip::tcp::endpoint& endpoint) :
        decorated_(decorated), endpoint_(endpoint) {}

    template <typename Body, typename Allocator, typename Send>
    void operator () (http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        std::string client_ip = endpoint_.address().to_string();

        LogRequest(client_ip, req);

        std::chrono::system_clock::time_point start_ts = std::chrono::system_clock::now();

        auto log_send = [this, client_ip, start_ts, send] (auto&& result) {

            int status_code = result.result_int();
            std::string content_type = "null";

            if (result.count(http::field::content_type)) {
                content_type = std::string(result[http::field::content_type]);
            }

            std::chrono::system_clock::time_point end_ts = std::chrono::system_clock::now();
            int response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_ts - start_ts).count();

            this->LogResponse(status_code, content_type, response_time, client_ip);

            send(std::move(result));
        };

        decorated_(std::move(req), std::move(log_send));

    }

private:

    void LogRequest(const std::string& client_ip, const StringRequest& req) {
        std::string uri = std::string(req.target());
        std::string method = std::string(beast::http::to_string(req.method()));

        json::value request_info = json::object{
            { "ip", client_ip },
            { "URI", uri },
            { "method", method}
        };

        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(data_attr, request_info) << "request received";
    
    }

    void LogResponse(int status_code, const std::string& content_type, int time, const std::string& client_ip) {
        json::value response_info = json::object{
            { "ip", client_ip },
            { "response_time", time },
            { "code", status_code },
            { "content_type", content_type}
        };

        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(data_attr, response_info) << "response sent";
    }

private:
     SomeRequestHandler& decorated_;
     const net::ip::tcp::endpoint& endpoint_;
};

}