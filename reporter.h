#pragma once

#include <vector>

#include "cli.h"
#include "runner.h"
#include "stats.h"

void print_report(const Config& cfg, const BenchmarkSummary& summary);
