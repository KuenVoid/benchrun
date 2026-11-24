#include "cli.h"
#include "stats.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

Config parse(std::initializer_list<std::string> arguments) {
    std::vector<std::string> values(arguments);
    std::vector<char*> argv;
    argv.reserve(values.size());
    for (auto& value : values)
        argv.push_back(value.data());
    return parse_cli(static_cast<int>(argv.size()), argv.data());
}

void test_cli_parsing() {
    const Config cfg = parse({"benchrun", "-n", "3", "--warmup", "2", "--json", "--", "/bin/echo", "hello"});
    assert(cfg.runs == 3);
    assert(cfg.warmup == 2);
    assert(cfg.json_output);
    assert((cfg.command == std::vector<std::string>{"/bin/echo", "hello"}));

    bool rejected = false;
    try {
        parse({"benchrun", "--unknown", "/bin/true"});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    Config conflicting = parse({"benchrun", "--stdin-file", "input", "--null-stdin", "--", "/bin/true"});
    rejected = false;
    try {
        validate_config(conflicting);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);
}

void test_statistics() {
    const std::vector<RunResult> results = {
        {.wall_ms = 1.0, .user_ms = 2.0, .sys_ms = 3.0, .peak_rss_bytes = 10, .outcome = RunOutcome::succeeded},
        {.wall_ms = 3.0, .user_ms = 4.0, .sys_ms = 5.0, .peak_rss_bytes = 30, .outcome = RunOutcome::succeeded},
        {.outcome = RunOutcome::exited_nonzero},
        {.outcome = RunOutcome::timed_out},
    };

    const BenchmarkSummary summary = compute_stats(results);
    assert(summary.total_runs == 4);
    assert(summary.succeeded == 2);
    assert(summary.failed_runs == 1);
    assert(summary.timed_out_runs == 1);
    assert(summary.wall_ms.min == 1.0);
    assert(summary.wall_ms.max == 3.0);
    assert(summary.wall_ms.mean == 2.0);
    assert(summary.wall_ms.median == 2.0);
    assert(std::abs(summary.wall_ms.stddev - std::sqrt(2.0)) < 1e-12);
    assert(summary.peak_rss_bytes.mean == 20.0);
}

} // namespace

int main() {
    test_cli_parsing();
    test_statistics();
}
