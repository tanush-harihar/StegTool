#pragma once

#include <string>
#include <vector>

namespace stegtool::cli {

class CLI {
public:
    CLI(int argc, char* argv[]);

    int run();

private:
    int argc_;
    char** argv_;

    void print_help() const;
    void print_version() const;
    void handle_embed();
    void handle_extract();
};

} // namespace stegtool::cli
