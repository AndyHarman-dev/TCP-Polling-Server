cmake_minimum_required(VERSION 3.14)
project(TCPEchoingServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)
# doctest v2.4.11 declares cmake_minimum_required(VERSION 2.8.12); CMake 4.x
# removed sub-3.5 compatibility so we set the floor before processing it.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(doctest)

add_library(tcp_echo_core STATIC
    util/sockets.cpp
    logging/Logger.cpp
    config/ArgParser.cpp
    net/Client.cpp
    net/ClientPool.cpp
    net/TcpServer.cpp
    app/ShutdownManager.cpp
    app/App.cpp
    db/DbLogger.cpp
)
target_include_directories(tcp_echo_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tcp_echo_core PRIVATE libpqxx::pqxx)

add_executable(TCPEchoingServer main.cpp)
target_link_libraries(TCPEchoingServer PRIVATE tcp_echo_core)

# libpqxx installed via Homebrew — no CMake config file shipped, so we locate
# headers and library manually and create an imported target.
find_path(LIBPQXX_INCLUDE_DIR pqxx/pqxx
    HINTS /opt/homebrew/include
    REQUIRED
)
find_library(LIBPQXX_LIB pqxx
    HINTS /opt/homebrew/lib
    REQUIRED
)
add_library(libpqxx::pqxx UNKNOWN IMPORTED)
set_target_properties(libpqxx::pqxx PROPERTIES
    IMPORTED_LOCATION "${LIBPQXX_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBPQXX_INCLUDE_DIR}"
)

add_executable(db_ping db/ping.cpp)
target_link_libraries(db_ping PRIVATE libpqxx::pqxx)

include(CTest)
add_executable(tests
    tests/test_main.cpp
    tests/test_sockets.cpp
    tests/test_logger.cpp
    tests/test_arg_parser.cpp
    tests/test_tcp_server.cpp
    tests/test_client.cpp
    tests/test_client_pool.cpp
    tests/test_shutdown_manager.cpp
    tests/test_app.cpp
    tests/test_db.cpp
    tests/test_db_errors.cpp
    tests/test_db_logger.cpp
)
target_link_libraries(tests PRIVATE tcp_echo_core doctest::doctest libpqxx::pqxx)
add_test(NAME tests COMMAND tests)
