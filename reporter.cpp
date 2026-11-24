#include "reporter.h"

#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <span>

namespace {
std::string format_duration(double ms) {
    if (ms < 0.0)
        ms = 0.0;
    double value = ms;
    std::string_view unit = " ms";

    if (ms < 1.0) {
        value = ms * 1000;
        unit = " us";
    } else if (ms >= 60000) {
        value = ms / 60000.0;
        unit = " min";
    } else if (ms >= 1000) {
        value = ms / 1000.0;
        unit = " s";
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value << unit;
    return oss.str();
}

std::string format_bytes(double bytes) {
    if (bytes < 0.0)
        bytes = 0.0;
    constexpr std::array<std::string_view, 4> units{" B", " KB", " MB", " GB"};
    std::size_t index = 0;
    while (bytes >= 1024 && index + 1 < units.size()) {
        bytes /= 1024;
        ++index;
    }

    std::ostringstream oss;
    if (index == 0)
        oss << static_cast<std::uint64_t>(bytes) << units[index];
    else
        oss << std::fixed << std::setprecision(2) << bytes << units[index];
    return oss.str();
}

void print_metric_row(std::string_view label, const MetricStats& m, bool is_bytes) {
    auto fmt = [is_bytes](double v) { return is_bytes ? format_bytes(v) : format_duration(v); };

    const std::string mean_std = fmt(m.mean) + " +- " + fmt(m.stddev);

    std::cout << std::left << std::setw(10) << label << std::setw(24) << mean_std << std::setw(14) << fmt(m.median) << std::setw(14)
              << fmt(m.min) << std::setw(14) << fmt(m.max) << '\n';
}

std::string json_escape(std::string_view value) {
    std::ostringstream oss;
    for (unsigned char ch : value) {
        switch (ch) {
        case '"': oss << "\\\""; break;
        case '\\': oss << "\\\\"; break;
        case '\b': oss << "\\b"; break;
        case '\f': oss << "\\f"; break;
        case '\n': oss << "\\n"; break;
        case '\r': oss << "\\r"; break;
        case '\t': oss << "\\t"; break;
        default:
            if (ch < 0x20) {
                oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<unsigned>(ch) << std::dec << std::setfill(' ');
            } else {
                oss << static_cast<char>(ch);
            }
        }
    }
    return oss.str();
}

const char* outcome_name(RunOutcome outcome) {
    switch (outcome) {
    case RunOutcome::succeeded: return "succeeded";
    case RunOutcome::exited_nonzero: return "exited_nonzero";
    case RunOutcome::signaled: return "signaled";
    case RunOutcome::timed_out: return "timed_out";
    }
    return "unknown";
}

void print_metric_json(std::ostream& out, std::string_view name, const MetricStats& metric) {
    out << "\"" << name << "\":{"
        << "\"min\":" << metric.min << ","
        << "\"max\":" << metric.max << ","
        << "\"mean\":" << metric.mean << ","
        << "\"median\":" << metric.median << ","
        << "\"stddev\":" << metric.stddev << "}";
}

void print_run_json(std::ostream& out, const RunResult& run) {
    out << "{"
        << "\"run_index\":" << run.run_index << ","
        << "\"phase\":\"" << (run.warmup ? "warmup" : "measured") << "\","
        << "\"start_time_ns\":" << run.start_time_ns << ","
        << "\"end_time_ns\":" << run.end_time_ns << ","
        << "\"outcome\":\"" << outcome_name(run.outcome) << "\","
        << "\"exit_code\":" << run.exit_code << ","
        << "\"term_signal\":" << run.term_signal << ","
        << "\"wall_ms\":" << run.wall_ms << ","
        << "\"user_ms\":" << run.user_ms << ","
        << "\"sys_ms\":" << run.sys_ms << ","
        << "\"peak_rss_bytes\":" << run.peak_rss_bytes
        << "}";
}

void print_runs_json(std::ostream& out, std::span<const RunResult> warmups,
                     std::span<const RunResult> measured) {
    out << "[";
    bool first = true;
    for (const RunResult& run : warmups) {
        if (!first) out << ",";
        print_run_json(out, run);
        first = false;
    }
    for (const RunResult& run : measured) {
        if (!first) out << ",";
        print_run_json(out, run);
        first = false;
    }
    out << "]";
}

void print_json(const Config& cfg, const BenchmarkSummary& summary,
                std::span<const RunResult> warmups,
                std::span<const RunResult> measured) {
    std::cout << "{"
              << "\"schema_version\":1,"
              << "\"command\":[";
    for (std::size_t i = 0; i < cfg.command.size(); ++i) {
        if (i > 0) std::cout << ",";
        std::cout << "\"" << json_escape(cfg.command[i]) << "\"";
    }
    std::cout << "],"
              << "\"configuration\":{\"runs\":" << cfg.runs
              << ",\"warmup\":" << cfg.warmup
              << ",\"quiet\":" << (cfg.quiet ? "true" : "false")
              << ",\"shell\":" << (cfg.use_shell ? "true" : "false")
              << ",\"timeout_ms\":" << cfg.timeout_ms
              << ",\"max_mem_mb\":" << cfg.max_mem_mb
              << ",\"cwd\":\"" << json_escape(cfg.cwd) << "\""
              << ",\"clear_env\":" << (cfg.clear_env ? "true" : "false")
              << ",\"null_stdin\":" << (cfg.null_stdin ? "true" : "false")
              << ",\"stdin_file\":\"" << json_escape(cfg.stdin_file) << "\"},"
              << "\"status\":{\"total\":" << summary.total_runs
              << ",\"succeeded\":" << summary.succeeded
              << ",\"failed\":" << summary.failed_runs
              << ",\"timed_out\":" << summary.timed_out_runs << "},"
              << "\"metrics\":{\"aggregation\":\"successful_runs\",";
    print_metric_json(std::cout, "wall_ms", summary.wall_ms);
    std::cout << ",";
    print_metric_json(std::cout, "user_ms", summary.user_ms);
    std::cout << ",";
    print_metric_json(std::cout, "sys_ms", summary.sys_ms);
    std::cout << ",";
    print_metric_json(std::cout, "peak_rss_bytes", summary.peak_rss_bytes);
    std::cout << "},"
              << "\"runs\":";
    print_runs_json(std::cout, warmups, measured);
    std::cout << "}\n";
}

void print_runs_json_only(std::span<const RunResult> warmups,
                          std::span<const RunResult> measured) {
    std::cout << "{\"schema_version\":1,\"runs\":";
    print_runs_json(std::cout, warmups, measured);
    std::cout << "}\n";
}

void print_csv(std::span<const RunResult> measured) {
    std::cout << "run_index,phase,outcome,exit_code,term_signal,wall_ms,user_ms,sys_ms,peak_rss_bytes,start_time_ns,end_time_ns\n";
    for (const RunResult& run : measured) {
        std::cout << run.run_index << ",measured,"
                  << outcome_name(run.outcome) << ","
                  << run.exit_code << "," << run.term_signal << ","
                  << run.wall_ms << "," << run.user_ms << "," << run.sys_ms << ","
                  << run.peak_rss_bytes << "," << run.start_time_ns << ","
                  << run.end_time_ns << "\n";
    }
}

void print_verbose(std::span<const RunResult> warmups,
                   std::span<const RunResult> measured) {
    std::cout << "\nPer-run results:\n";
    auto print_phase = [](std::string_view name, std::span<const RunResult> runs) {
        for (const RunResult& run : runs) {
            std::cout << name << " " << run.run_index + 1 << ": "
                      << outcome_name(run.outcome)
                      << " exit=" << run.exit_code
                      << " signal=" << run.term_signal
                      << " wall_ms=" << run.wall_ms
                      << " user_ms=" << run.user_ms
                      << " sys_ms=" << run.sys_ms
                      << " peak_rss_bytes=" << run.peak_rss_bytes << '\n';
        }
    };
    print_phase("warmup", warmups);
    print_phase("run", measured);
}

} // namespace

void print_report(const Config& cfg, const BenchmarkSummary& summary) {
    std::cout << "Benchmark: ";
    for (std::size_t i = 0; i < cfg.command.size(); ++i) {
        if (i > 0)
            std::cout << ' ';
        std::cout << cfg.command[i];
    }
    std::cout << " (" << cfg.runs << " runs, " << cfg.warmup << " warmup)\n";

    std::cout << "Status:    " << summary.succeeded << '/' << summary.total_runs << " succeeded";
    if (summary.failed_runs > 0 || summary.timed_out_runs > 0) {
        std::cout << " (" << summary.failed_runs << " failed, " << summary.timed_out_runs << " timed out)";
    }
    std::cout << "\n\n";

    std::cout << std::left << std::setw(10) << "Metric" << std::setw(24) << "Mean +- Stddev" << std::setw(14) << "Median"
              << std::setw(14) << "Min" << std::setw(14) << "Max" << '\n'
              << std::string(76, '-') << '\n';

    print_metric_row("Wall", summary.wall_ms, false);
    print_metric_row("User", summary.user_ms, false);
    print_metric_row("Sys", summary.sys_ms, false);
    print_metric_row("Peak RSS", summary.peak_rss_bytes, true);
}

void print_report(const Config& cfg, const BenchmarkSummary& summary,
                  std::span<const RunResult> warmups,
                  std::span<const RunResult> measured) {
    if (cfg.runs_json) {
        print_runs_json_only(warmups, measured);
    } else if (cfg.json_output) {
        print_json(cfg, summary, warmups, measured);
    } else if (cfg.csv_output) {
        print_csv(measured);
    } else {
        print_report(cfg, summary);
        if (cfg.verbose)
            print_verbose(warmups, measured);
    }
}
