#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/** Complete command-line configuration for one benchrun invocation. */
struct Config {
    std::size_t runs = 1;
    std::size_t warmup = 0;
    bool quiet = false;
    bool use_shell = false;
    bool json_output = false;
    bool verbose = false;
    bool runs_json = false;
    bool csv_output = false;
    bool clear_env = false;
    bool null_stdin = false;
    bool show_help = false;
    std::uint64_t timeout_ms = 0;
    std::size_t max_mem_mb = 0;
    std::string cwd;
    std::string stdin_file;
    std::vector<std::string> env;
    std::vector<std::string> command;
};

/**
 * Parses benchmark flags and target arguments from argc/argv.
 * Target arguments may follow -- without being interpreted as benchrun flags.
 * Parsing failures are reported as std::invalid_argument exceptions.
 */
Config parse_cli(int argc, char* argv[]);

/**
 * Validates cross-field configuration constraints before benchmark execution.
 * Validation failures are reported as std::invalid_argument exceptions.
 */
void validate_config(const Config& cfg);

/**
 * Prints usage and all supported command-line options to standard output.
 * prog_name is used as the executable name; a null or empty name uses
 * "benchrun".
 */
void print_help(const char* prog_name);
