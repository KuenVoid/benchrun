#include <cstdlib>
#include <exception>
#include <iostream>

#include "cli.h"
#include "reporter.h"
#include "runner.h"
#include "stats.h"

int main(int argc, char* argv[]) {
    Config cfg;
    try {
        cfg = parse_cli(argc, argv);
        validate_config(cfg);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        print_help(argv[0]);
        return 1;
    }

    if (cfg.show_help) {
        print_help(argv[0]);
        return 0;
    }

    if (cfg.json_output || cfg.runs_json || cfg.csv_output)
        cfg.quiet = true;

    const auto execution = run_benchmark(cfg);
    if (execution.error) {
        std::cerr << "benchrun: " << execution.error->operation << " failed";
        if (!execution.error->message.empty())
            std::cerr << ": " << execution.error->message;
        std::cerr << '\n';
        return 1;
    }

    const BenchmarkSummary summary = compute_stats(execution.results.measured);
    if (!print_report(cfg, summary, execution.results.warmups, execution.results.measured))
        return 1;
    return (summary.failed_runs > 0 || summary.timed_out_runs > 0) ? 1 : 0;
}
