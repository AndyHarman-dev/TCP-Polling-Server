#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

std::unordered_map<std::string, std::string> parse_args(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> args;
    int positional = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) == 0) {
            std::string body = arg.substr(2);
            auto eq = body.find('=');
            if (eq != std::string::npos) {
                args[body.substr(0, eq)] = body.substr(eq + 1);
            } else if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) {
                args[body] = argv[++i];
            } else {
                args[body] = "";
            }
        } else {
            args[std::to_string(positional++)] = arg;
        }
    }
    return args;
}

std::filesystem::path glog_filepath = "./server_msgs.log";

int main(int argc, char* argv[]) {

    auto args = parse_args(argc, argv);
    if (args.empty()) {
        std::cerr << "Usage: server <port> [options]" << std::endl;
        return 1;
    }

    if (args.size() == 1) {
        std::printf("No log output file was provided. Dumping locally at %s\n", glog_filepath.string().c_str());
    }
    else if (args.size() == 2) {
        glog_filepath = args["log-file"];
        std::printf("%s\n", glog_filepath.c_str());
    }
}
