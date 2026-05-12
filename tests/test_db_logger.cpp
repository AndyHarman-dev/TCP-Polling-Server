//
// Created by King Andrés on 12/05/2026.
//

#include <doctest/doctest.h>
#include "db/DbLogger.h"

TEST_CASE("DbLogger: construct logger") {
    CHECK_THROWS_AS(DbLogger(
        "host=gdasg dbname=sdgsdg user=sdgds application_name=my_server"
    ), std::runtime_error);
}

TEST_CASE("DbLogger: log") {
    DbLogger logger(
        "host=localhost dbname=app_db user=appuser application_name=my_server"
    );

    const std::string StartupMessage("log test: server booted");
    const std::string TcpMessage("log test: slow client");

    {
        pqxx::connection conn("host=localhost dbname=app_db user=appuser application_name=my_server");
        pqxx::work w{conn};
        w.exec("TRUNCATE logs");
        w.commit();
    }

    logger.log(
        DbLogger::Level::Info,
        "startup", StartupMessage
    );

    logger.log(
        DbLogger::Level::Warning,
        "tcp", TcpMessage,
        std::string("203.0.113.42"),
        R"({"latency_ms": 1240})"
    );

    pqxx::connection conn("host=localhost dbname=app_db user=appuser application_name=my_server");
    pqxx::work w{conn};

    bool bHasStartupSourceAndMessage = false;
    bool bHasTcpSourceAndMessage = false;
    auto result = w.exec("SELECT source, message FROM logs");

    for (const auto& row: result) {
        auto source = row["source"].as<std::string>();
        auto msg = row["message"].as<std::string>();

        if (source == "startup" && msg == StartupMessage)
            bHasStartupSourceAndMessage = true;

        if (source == "tcp" && msg == TcpMessage)
            bHasTcpSourceAndMessage = true;
    }

    CHECK(bHasStartupSourceAndMessage);
    CHECK(bHasTcpSourceAndMessage);
}