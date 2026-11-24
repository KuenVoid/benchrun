#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "runner.h"

/** Statistical summary of a single benchmark metric. */
struct MetricStats {
    double min = 0.0;
    double max = 0.0;
    double mean = 0.0;
    double median = 0.0;
    double stddev = 0.0;
};

/** Aggregate metrics and run-status counts for a benchmark invocation. */
struct BenchmarkSummary {
    MetricStats wall_ms;
    MetricStats user_ms;
    MetricStats sys_ms;
    MetricStats peak_rss_bytes;
    std::size_t total_runs = 0;
    std::size_t failed_runs = 0;
    std::size_t timed_out_runs = 0;
    std::size_t succeeded = 0;
};

/** Selects which run outcomes contribute values to metric aggregates. */
enum class AggregationPolicy {
    /** Include successful executions only in metric aggregates. */
    successful_runs,
};

/**
 * Computes aggregate statistics for a run collection.
 * policy controls metric inclusion; status counters always cover every input
 * run. The default policy includes successful runs only.
 */
BenchmarkSummary compute_stats(std::span<const RunResult> results,
                               AggregationPolicy policy = AggregationPolicy::successful_runs);
