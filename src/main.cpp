#include "sdk.h"
//
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>

#include "app.h"
#include "infrastructure.h"
#include "json_loader.h"
#include "request_handler.h"
#include "logger.h"
#include "postgres.h"
#include "ticker.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
namespace po = boost::program_options;

namespace {

struct CommandLineArgs {
    std::string config_file;
    std::string static_dir;
    std::string state_file;
    int tick_period;
    int save_state_period;
    bool randomize = false;
    bool state_file_exist = false;
};

std::optional<CommandLineArgs> ParseCommandLine(int argc, const char* argv[]) {
    po::options_description desc("Allowed options");

    CommandLineArgs args;
    desc.add_options()
        ("help,h", "produced help message")
        ("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config path")
        ("www-root,w", po::value(&args.static_dir)->value_name("dir"s), "set static file root")
        ("state-file,f", po::value(&args.state_file)->value_name("file"s), "set state file")
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")
        ("save-state-period,s", po::value(&args.save_state_period)->value_name("milliseconds"s), "set save state period")
        ("randomize-spawn-dogs", "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file have not been specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static directory have not been specified"s);
    }
    if(vm.contains("state-file")) {
        args.state_file_exist = true;
    } 
    if(!vm.contains("tick-period")) {
        args.tick_period = -1;
    }
    if(!vm.contains("save-state-period")) {
        args.save_state_period = -1;
    } 
    if(vm.contains("randomize-spawn-points")) {
        args.randomize = true;
    }
    return args;
}

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace


int main(int argc, const char* argv[]) {
    InitCustomConsoleLog();

    try {
        auto args = ParseCommandLine(argc, argv);
        if (!args) {
            return EXIT_FAILURE;
        }

        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(args->config_file);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                logger::LogServerStop();
                ioc.stop();
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        net::ip::address address = net::ip::address_v4::any();
        unsigned short port = 8080;
        net::ip::tcp::endpoint endpoint(address, port);

        std::filesystem::path static_path = args->static_dir;

        app::Players players;
        app::Application application(game, players, postgres_database::GetConfigFromEnv());

        if (args->state_file_exist) {
            if (args->save_state_period != -1) {
                auto ser_list_ptr = std::make_unique<infrastructure::SerializingListener>(application, std::chrono::milliseconds(args->save_state_period));
                ser_list_ptr->SetSerializeFile(args->state_file);
                application.SetApplicationListener(std::move(ser_list_ptr));
            }
            serialization::AppDeserialization(args->state_file, application);
        }

        net::strand<net::io_context::executor_type> api_strand{net::make_strand(ioc)};

        if (args->randomize) {
            application.SetGenerateRandPos(true);
        }
        
        http_handler::RequestHandler handler{ game, static_path, application, api_strand };
        logger::LoggingRequestHandler log_handler(handler, endpoint);

        // 5. Если указан tick-period, создаем автоматический тикер
        std::shared_ptr<Ticker> ticker;
        if (args->tick_period != -1) {
            auto period = std::chrono::milliseconds(args->tick_period);
            ticker = std::make_shared<Ticker>(api_strand, period, 
                [&application](std::chrono::milliseconds delta) {
                    application.Tick(delta);
                });
            application.SetAutoTickEnabled(true);
            ticker->Start();
        }

        // 6. Запустить обработчик HTTP-запросов
        http_server::ServeHttp(ioc, endpoint, [&log_handler](auto&& req, auto&& send) {
            log_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        logger::LogServerStart(port, address);

        // 7. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        if (args->state_file_exist) {
            serialization::AppSerialization(args->state_file, application);
        }

    } catch (const std::exception& ex) {
        logger::LogServerStopEx(ex, EXIT_FAILURE);
        return EXIT_FAILURE;
    }
}
