//
// Created by King Andrés on 12/05/2026.
//

#include "DbLoggerProxy.h"

DbLoggerProxy::DbLoggerProxy(std::string_view connection_string, std::string_view source, std::optional<std::string> client) :
inner_(connection_string),
source_(source),
client_(std::move(client))
{}

void DbLoggerProxy::info(std::string_view msg) {
    inner_.log(
        DbLogger::Level::Info,
        source_,
        msg,
        client_,
        {}
    );
}

void DbLoggerProxy::error(std::string_view msg) {
    inner_.log(
        DbLogger::Level::Error,
        source_,
        msg,
        client_,
        {}
    );
}

void DbLoggerProxy::raw(const char *data, int n) {
    info(std::string(data));
}
