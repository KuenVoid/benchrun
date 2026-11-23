#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct Config {
    std::size_t runs = 1;
    std::size_t warmup = 0;
    bool quiet = false;
    bool use_shell = false;
    bool json_output = false;
    bool show_help = false;
    std::uint64_t timeout_ms = 0;
    std::size_t max_mem_mb = 0;
    std::vector<std::string> command;
};

Config parse_cli(int argc, char* argv[]);
void validate_config(const Config& cfg);
void print_help(const char* prog_name);
