#include <doctest/doctest.h>
#include <pqxx/pqxx>

static const char* DSN = "dbname=pqxx_learn";

TEST_CASE("bad connection string throws") {
    CHECK_THROWS_AS(
        pqxx::connection{"dbname=does_not_exist"},
        pqxx::broken_connection
    );
}

TEST_CASE("constraint violation throws sql_error and rolls back") {
    pqxx::connection conn{DSN};

    // Setup: table with a UNIQUE constraint
    {
        pqxx::work tx{conn};
        tx.exec("DROP TABLE IF EXISTS unique_test");
        tx.exec("CREATE TABLE unique_test (name TEXT UNIQUE NOT NULL)");
        tx.exec_params("INSERT INTO unique_test VALUES ($1)", "alice");
        tx.commit();
    }

    // Attempt a duplicate insert — must throw
    {
        pqxx::work tx{conn};
        CHECK_THROWS_AS(
            tx.exec_params("INSERT INTO unique_test VALUES ($1)", "alice"),
            pqxx::unique_violation   // a subclass of pqxx::sql_error
        );
        // tx is destroyed here without commit() → automatic ROLLBACK
    }

    // Table must still have only one row
    {
        pqxx::nontransaction tx{conn};
        auto r = tx.exec("SELECT COUNT(*) AS n FROM unique_test");
        CHECK(r[0]["n"].as<int>() == 1);
    }
}
