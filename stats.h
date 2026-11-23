#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "runner.h"

struct MetricStats {
    double min = 0.0;
    double max = 0.0;
    double mean = 0.0;
    double median = 0.0;
    double stddev = 0.0;
};

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

enum class AggregationPolicy {
    successful_runs,
};

BenchmarkSummary compute_stats(std::span<const RunResult> results,
                               AggregationPolicy policy = AggregationPolicy::successful_runs);
