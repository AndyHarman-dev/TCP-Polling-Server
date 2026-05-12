//
// Created by King Andrés on 12/05/2026.
//

#include "DbLogger.h"
#include <iostream>

void DbLogger::ensure_schema() {
   pqxx::work txn{ *conn_ };
   txn.exec(R"sql(
            CREATE TABLE IF NOT EXISTS logs (
                id BIGSERIAL PRIMARY KEY,
                ts TIMESTAMPTZ NOT NULL DEFAULT NOW(),
                level TEXT NOT NULL,
                source TEXT,
                client INET,
                message TEXT NOT NULL,
                metadata JSONB
            )
        )sql");
   txn.commit();
}

DbLogger::DbLogger(std::string_view conn_str) {
   conn_ = std::make_unique<pqxx::connection>(conn_str.data());
   ensure_schema();
   conn_->prepare("insert_log", "INSERT INTO logs (level, source, client, message, metadata) "
                                "VALUES ($1, $2, $3::inet, $4, $5::jsonb)");
}

void DbLogger::log(Level l, std::string_view source, std::string_view message, std::optional<std::string> client,
   std::optional<std::string> json_obj) {
   std::lock_guard<std::mutex> lk(mtx_);
   try {
      pqxx::work txn{ *conn_ };
      txn.exec_prepared("insert_log",
         level_str(l),
         std::string(source),
         client.has_value() ? pqxx::params{ *client } : pqxx::params{nullptr},
         std::string(message),
         json_obj.has_value() ? pqxx::params{ *json_obj } : pqxx::params{nullptr}
      );

      txn.commit();
   }
   catch (const pqxx::broken_connection& e) {
      std::cerr << e.what() << std::endl;
   }
}

std::string DbLogger::level_str(Level l) {
   switch (l) {
      case Info: return "INFO";
      case Warning: return "WARNING";
      case Error: return "ERROR";
      case Trace: return "TRACE";
      case Debug: return "DEBUG";
      case Fatal: return "FATAL";
      default: return "Unknown" ;
   }
}
