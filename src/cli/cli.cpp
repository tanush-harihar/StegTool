#include "cli.hpp"
#include "../core/exceptions.hpp"
#include <iostream>

namespace stegtool::cli {

CLI::CLI(int argc, char* argv[]) : argc_(argc), argv_(argv) {}

int CLI::run() {
    try {
        if (argc_ < 2) {
            print_help();
            return 1;
        }

        std::string command = argv_[1];

        if (command == "help" || command == "--help" || command == "-h") {
            print_help();
            return 0;
        } else if (command == "version" || command == "--version" || command == "-v") {
            print_version();
            return 0;
        } else if (command == "embed") {
            handle_embed();
            return 0;
        } else if (command == "extract") {
            handle_extract();
            return 0;
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            print_help();
            return 1;
        }
    } catch (const StegtoolException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}

void CLI::print_help() const {
    std::cout << "stegtool - Terminal-Based Steganography Tool\n"
              << "Usage: stegtool [COMMAND] [OPTIONS]\n\n"
              << "Commands:\n"
              << "  embed       Embed a secret file into a carrier\n"
              << "  extract     Extract a secret file from a carrier\n"
              << "  help        Show this help message\n"
              << "  version     Show version information\n\n"
              << "Options:\n"
              << "  -h, --help       Show help message\n"
              << "  -v, --version    Show version\n"
              << std::endl;
}

void CLI::print_version() const {
    std::cout << "stegtool version 1.0.0\n"
              << "A modular steganography tool\n"
              << std::endl;
}

void CLI::handle_embed() {
    // TODO: Implement embed command
    std::cerr << "Embed command not yet implemented\n";
}

void CLI::handle_extract() {
    // TODO: Implement extract command
    std::cerr << "Extract command not yet implemented\n";
}

} // namespace stegtool::cli
