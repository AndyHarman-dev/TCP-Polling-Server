#include <iostream>
#include <stdexcept>
#include "app/App.h"
#include "config/ArgParser.h"
#include "config/Config.h"

int main(int argc, char* argv[]) {
    Config cfg;
    try {
        ArgParser(argc, argv).apply(cfg);
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    try {
        return App(cfg).run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
