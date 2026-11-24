#pragma once

#include <span>
#include <vector>

#include "cli.h"
#include "runner.h"
#include "stats.h"

/**
 * Prints a benchmark summary using the format selected by cfg.
 * cfg supplies command context and output options; summary supplies aggregate
 * metrics and status counts.
 */
bool print_report(const Config& cfg, const BenchmarkSummary& summary);

/**
 * Prints the summary and, when requested, individual run records.
 * warmups and measured contain the records from the corresponding benchmark
 * phases.
 */
bool print_report(const Config& cfg, const BenchmarkSummary& summary,
                  std::span<const RunResult> warmups,
                  std::span<const RunResult> measured);
