#include "cli.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace {
struct FlagSpec {
    std::string_view short_name;
    std::string_view long_name;
    std::string_view value_hint;
    std::string_view description;
    bool takes_value;
    void (*apply)(Config&, std::string_view, std::string_view);
};

std::uint64_t parse_u64(std::string_view sv, std::string_view flag_name) {
    if (sv.empty())
        throw std::invalid_argument(std::string(flag_name) + " should be non-empty");
    std::uint64_t output = 0;
    const char* begin = sv.data();
    const char* end = begin + sv.size();
    auto [ptr, ec] = std::from_chars(begin, end, output);

    if (ec == std::errc::result_out_of_range)
        throw std::invalid_argument(std::string(flag_name) + " is out of range");
    if (ec != std::errc{} || ptr != end)
        throw std::invalid_argument(std::string(flag_name) + " must be a non-negative integer");
    return output;
}

template <bool Config::* Field> void set_flag(Config& cfg, std::string_view, std::string_view) {
    cfg.*Field = true;
}

template <std::string Config::* Field> void set_string(Config& cfg, std::string_view val, std::string_view) {
    cfg.*Field = std::string(val);
}

void set_env(Config& cfg, std::string_view val, std::string_view flag_name) {
    if (val.find('=') == std::string_view::npos)
        throw std::invalid_argument(std::string(flag_name) + " requires KEY=VALUE");
    cfg.env.emplace_back(val);
}

template <auto Config::* Field, std::uint64_t Min = 0> void set_u64(Config& cfg, std::string_view val, std::string_view flag_name) {
    auto parsed = parse_u64(val, flag_name);
    if (parsed < Min)
        throw std::invalid_argument(std::string(flag_name) + " must be >= " + std::to_string(Min));
    using MemberT = std::remove_reference_t<decltype(cfg.*Field)>;
    if (parsed > std::numeric_limits<MemberT>::max())
        throw std::invalid_argument(std::string(flag_name) + " is out of range");
    cfg.*Field = static_cast<MemberT>(parsed);
}

constexpr std::array<FlagSpec, 17> kFlags = {{
    {"-r", "--runs", "<int>", "Number of benchmark runs (default: 1)", true, &set_u64<&Config::runs, 1>},
    {"-n", "--runs", "<int>", "Alias for -r/--runs", true, &set_u64<&Config::runs, 1>},
    {"-w", "--warmup", "<int>", "Number of warmup runs (default: 0)", true, &set_u64<&Config::warmup>},
    {"-q", "--quiet", "", "Suppress child stdout and stderr", false, &set_flag<&Config::quiet>},
    {"-s", "--shell", "", "Run command via /bin/sh -c", false, &set_flag<&Config::use_shell>},
    {"-t", "--timeout", "<ms>", "Kill child after timeout in ms (0 = off)", true, &set_u64<&Config::timeout_ms>},
    {"-m", "--max-mem", "<MB>", "Memory limit in MB via RLIMIT_AS (0 = off)", true, &set_u64<&Config::max_mem_mb>},
    {"", "--json", "", "Write benchmark results to report.json", false, &set_flag<&Config::json_output>},
    {"", "--verbose", "", "Print individual warmup and measured runs", false, &set_flag<&Config::verbose>},
    {"", "--runs-json", "", "Write per-run JSON records to report.json", false, &set_flag<&Config::runs_json>},
    {"", "--csv", "", "Write measured per-run records to report.csv", false, &set_flag<&Config::csv_output>},
    {"", "--cwd", "<path>", "Set the child working directory", true, &set_string<&Config::cwd>},
    {"", "--env", "<KEY=VALUE>", "Set a child environment variable (repeatable)", true, &set_env},
    {"", "--clear-env", "", "Clear the inherited environment", false, &set_flag<&Config::clear_env>},
    {"", "--stdin-file", "<path>", "Read child stdin from a file", true, &set_string<&Config::stdin_file>},
    {"", "--null-stdin", "", "Connect child stdin to /dev/null", false, &set_flag<&Config::null_stdin>},
    {"-h", "--help", "", "Show this help message", false, &set_flag<&Config::show_help>},
}};

const FlagSpec* find_flag(std::string_view token) {
    for (const auto& spec : kFlags) {
        if ((!spec.short_name.empty() && token == spec.short_name) || (!spec.long_name.empty() && token == spec.long_name))
            return &spec;
    }
    return nullptr;
}

} // namespace

Config parse_cli(int argc, char** argv) {
    Config result;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--") {
            for (int n = i + 1; n < argc; ++n)
                result.command.push_back(argv[n]);
            break;
        }

        if (const FlagSpec* spec = find_flag(arg)) {
            std::string_view val;
            if (spec->takes_value) {
                if (i + 1 >= argc)
                    throw std::invalid_argument(std::string(arg) + " requires a value");
                val = argv[++i];
            }
            spec->apply(result, val, arg);
            continue;
        }

        if (!arg.empty() && arg.front() == '-' && arg != "-")
            throw std::invalid_argument("unknown flag: " + std::string(arg));

        for (int n = i; n < argc; ++n)
            result.command.push_back(argv[n]);

        if (result.command.empty() && !result.show_help)
            throw std::invalid_argument("missing command");

        break;
    }
    if (result.command.empty() && !result.show_help)
        throw std::invalid_argument("missing command");
    return result;
}

void validate_config(const Config& cfg) {
    if (cfg.show_help)
        return;
    if (cfg.command.empty())
        throw std::invalid_argument("missing command");
    if (cfg.runs == 0)
        throw std::invalid_argument("--runs must be >= 1");
    if (!cfg.stdin_file.empty() && cfg.null_stdin)
        throw std::invalid_argument("--stdin-file and --null-stdin cannot be used together");
}

void print_help(const char* prog_name) {
    const char* name = (prog_name && prog_name[0] != '\0') ? prog_name : "benchrun";
    std::cout << "Usage: " << name << "[OPTIONS] [--] <command> [args]\n\n" << "Options:\n";
    constexpr int kFlagColWidth = 24;
    for (const auto& spec : kFlags) {
        std::string left = spec.short_name.empty() ? "    " + std::string(spec.long_name)
                                                   : std::string(spec.short_name) + ", " + std::string(spec.long_name);
        if (!spec.value_hint.empty()) {
            left += ' ';
            left += spec.value_hint;
        }
        std::cout << " " << std::left << std::setw(kFlagColWidth) << left << spec.description << '\n';
    }
}
