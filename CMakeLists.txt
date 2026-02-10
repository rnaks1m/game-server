cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(MyLib STATIC
    src/collision_detector.h
    src/collision_detector.cpp
    src/geom.h
    src/extra_data.h
    src/extra_data.cpp
    src/loot_generator.h
    src/loot_generator.cpp
    src/model.h
    src/model.cpp
    src/model_serialization.h
    src/tagged.h
)

target_include_directories(MyLib PUBLIC 
    CONAN_PKG::boost
)
target_link_libraries(MyLib PUBLIC 
    CONAN_PKG::boost 
    Threads::Threads
)

add_executable(game_server
    src/api_handler.h
    src/api_handler.cpp
    src/app.h
    src/app.cpp
    src/app_serialization.h
    src/app_serialization.cpp
    src/boost_json.cpp
    src/http_server.h
    src/http_server.cpp
    src/infrastructure.h
    src/json_loader.h
    src/json_loader.cpp
    src/logger.h
    src/logger.cpp
    src/main.cpp
    src/postgres.h
    src/postgres.cpp
    src/request_handler.h
    src/request_handler.cpp
    src/retired_player.h
    src/retired_player.cpp
    src/sdk.h
    src/tagged_uuid.h
    src/tagged_uuid.cpp
    src/ticker.h
)

add_executable(game_server_tests
    tests/loot_generator_tests.cpp
    tests/collision-detector-tests.cpp
    tests/state-serialization-tests.cpp
)

target_link_libraries(game_server MyLib CONAN_PKG::libpq CONAN_PKG::libpqxx)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 MyLib)

include(CTest)
if(BUILD_TESTING)
    include(CTest)
endif()  