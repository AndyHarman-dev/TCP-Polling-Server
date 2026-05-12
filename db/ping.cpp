#include <pqxx/pqxx>
#include <iostream>

int main() {
    pqxx::connection conn{"dbname=pqxx_learn"};
    pqxx::nontransaction tx{conn};
    pqxx::result r = tx.exec("SELECT version()");
    std::cout << r[0][0].as<std::string>() << "\n";
}
