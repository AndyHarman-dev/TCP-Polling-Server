#include <doctest/doctest.h>
#include <pqxx/pqxx>

// Shared connection string for all db tests
static const char* DSN = "dbname=pqxx_learn";

// Runs before each test case to give us a clean table
static void reset_table(pqxx::connection& conn) {
    pqxx::work tx{conn};
    tx.exec("DROP TABLE IF EXISTS fruits");
    tx.exec(R"(
        CREATE TABLE fruits (
            id    SERIAL PRIMARY KEY,
            name  TEXT NOT NULL,
            color TEXT NOT NULL
        )
    )");
    tx.commit();
}

TEST_CASE("database connection and SELECT 1") {
    pqxx::connection conn{DSN};
    CHECK(conn.is_open());

    pqxx::nontransaction tx{conn};
    pqxx::result r = tx.exec("SELECT 1 AS answer");

    REQUIRE(r.size() == 1);
    CHECK(r[0]["answer"].as<int>() == 1);
}

TEST_CASE("INSERT and SELECT") {
    pqxx::connection conn{DSN};
    reset_table(conn);

    // --- INSERT ---
    {
        pqxx::work tx{conn};
        // exec0 asserts the query affects no rows (DDL/non-SELECT safety check)
        // exec_params sends parameters safely — never concatenate user input into SQL
        tx.exec_params("INSERT INTO fruits (name, color) VALUES ($1, $2)", "apple",  "red");
        tx.exec_params("INSERT INTO fruits (name, color) VALUES ($1, $2)", "banana", "yellow");
        tx.commit();   // <-- nothing is written to disk until this line
    }

    // --- SELECT ---
    {
        pqxx::work tx{conn};
        pqxx::result r = tx.exec("SELECT name, color FROM fruits ORDER BY name");
        tx.commit();

        REQUIRE(r.size() == 2);
        CHECK(r[0]["name"].as<std::string>()  == "apple");
        CHECK(r[0]["color"].as<std::string>() == "red");
        CHECK(r[1]["name"].as<std::string>()  == "banana");
    }
}

TEST_CASE("UPDATE") {
    pqxx::connection conn{DSN};
    reset_table(conn);

    {
        pqxx::work tx{conn};
        tx.exec_params("INSERT INTO fruits (name, color) VALUES ($1, $2)", "apple", "green");
        tx.commit();
    }

    {
        pqxx::work tx{conn};
        pqxx::result r = tx.exec_params(
            "UPDATE fruits SET color = $1 WHERE name = $2 RETURNING id, color",
            "red", "apple"
        );
        tx.commit();

        REQUIRE(r.size() == 1);
        CHECK(r[0]["color"].as<std::string>() == "red");
    }
}

TEST_CASE("DELETE") {
    pqxx::connection conn{DSN};
    reset_table(conn);

    {
        pqxx::work tx{conn};
        tx.exec_params("INSERT INTO fruits (name, color) VALUES ($1, $2)", "apple",  "red");
        tx.exec_params("INSERT INTO fruits (name, color) VALUES ($1, $2)", "banana", "yellow");
        tx.commit();
    }

    {
        pqxx::work tx{conn};
        tx.exec_params("DELETE FROM fruits WHERE name = $1", "banana");
        tx.commit();
    }

    {
        pqxx::nontransaction tx{conn};
        pqxx::result r = tx.exec("SELECT COUNT(*) AS n FROM fruits");
        CHECK(r[0]["n"].as<int>() == 1);
    }
}
