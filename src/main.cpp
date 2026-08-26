#include "cli/cli.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        stegtool::cli::CLI cli(argc, argv);
        return cli.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
