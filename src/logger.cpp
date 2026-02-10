#include "logger.h"

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    // Момент времени приходится вручную конвертировать в строку.
    // Для получения истинного значения атрибута нужно добавить
    // разыменование. 
    auto ts = *rec[timestamp];
    strm << "{\"timestamp\":\"" << to_iso_extended_string(ts) << "\",";

    // Выводим данные
    auto data = rec[data_attr];
    strm << "\"data\":" << json::serialize(*data) << ",";

    // Выводим само сообщение.
    auto message = rec[message_attr];
    strm << "\"message\":" << "\"" << rec[logging::expressions::smessage] << "\"}"; 

}

void InitCustomConsoleLog() {
    logging::add_common_attributes();
    
    logging::add_console_log(
        std::clog,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
    );
}

namespace logger {

void LogServerStart(unsigned short port, net::ip::address address) {
    json::value run_info = json::object{
        { "port", port },
        { "address", address.to_string() }
    };
    BOOST_LOG_TRIVIAL(info) << boost::log::add_value(data_attr, run_info) << "server started";
}

void LogServerStop() {
    json::value stop_info = json::object{
        { "code", 0 }
    };
    BOOST_LOG_TRIVIAL(info) << boost::log::add_value(data_attr, stop_info) << "server exited";
}

void LogServerStopEx(const std::exception& ex, int code) {
    json::value exception_info = json::object{
        { "code", code },
        { "exception", ex.what() }
    };
    BOOST_LOG_TRIVIAL(info) << boost::log::add_value(data_attr, exception_info) << "server exited";
}

void LogServerError(const sys::error_code& ec, std::string where) {
    json::value exception_info = json::object{
        { "code", ec.value() },
        { "text", ec.message() },
        { "where", where}
    };
    BOOST_LOG_TRIVIAL(info) << boost::log::add_value(data_attr, exception_info) << "error";
}

}