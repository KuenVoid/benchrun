#include "runner.h"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <limits>
#include <string>
#include <sys/resource.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

extern char** environ;

namespace {
struct PreparedCommand {
    std::string shell_cmd;
    std::vector<char*> argv;
};

PreparedCommand prepare_command(const Config& cfg) {
    PreparedCommand output;
    if (!cfg.use_shell) {
        output.argv.reserve(cfg.command.size() + 1);
        for (const auto& arg : cfg.command)
            output.argv.push_back(const_cast<char*>(arg.c_str()));
        output.argv.push_back(nullptr);
        return output;
    }

    std::size_t total_len = cfg.command.empty() ? 0 : cfg.command.size() - 1;
    for (const auto& arg : cfg.command)
        total_len += arg.size();
    output.shell_cmd.reserve(total_len);
    for (std::size_t i = 0; i < cfg.command.size(); ++i) {
        if (i > 0)
            output.shell_cmd += ' ';
        output.shell_cmd += cfg.command[i];
    }
    output.argv = {
        const_cast<char*>("/bin/sh"),
        const_cast<char*>("-c"),
        output.shell_cmd.data(),
        nullptr,
    };
    return output;
}

RunnerError make_error(const char* operation, int error_code) {
    return {operation, error_code, std::strerror(error_code)};
}

std::uint64_t normalize_max_rss_byte(long ru_maxrss) {
    if (ru_maxrss < 0)
        return 0;
    const auto rss = static_cast<std::uint64_t>(ru_maxrss);
#ifdef __APPLE__
    return rss;
#else
    return rss * 1024;
#endif
}

void setup_child_process(const Config& cfg) {
    if (!cfg.cwd.empty() && chdir(cfg.cwd.c_str()) != 0) {
        perror("chdir");
        _exit(126);
    }

    if (cfg.clear_env) {
        std::vector<std::string> names;
        for (char** entry = environ; entry && *entry; ++entry) {
            const char* equal = std::strchr(*entry, '=');
            names.emplace_back(*entry, equal ? static_cast<std::size_t>(equal - *entry)
                                             : std::strlen(*entry));
        }
        for (const std::string& name : names) {
            if (unsetenv(name.c_str()) != 0) {
                perror("unsetenv");
                _exit(126);
            }
        }
    }

    for (const std::string& assignment : cfg.env) {
        const std::size_t equal = assignment.find('=');
        if (equal == std::string::npos)
            _exit(126);
        const std::string name = assignment.substr(0, equal);
        const std::string value = assignment.substr(equal + 1);
        if (setenv(name.c_str(), value.c_str(), 1) != 0) {
            perror("setenv");
            _exit(126);
        }
    }

    if (cfg.null_stdin || !cfg.stdin_file.empty()) {
        const char* path = cfg.null_stdin ? "/dev/null" : cfg.stdin_file.c_str();
        const int fd = open(path, O_RDONLY);
        if (fd < 0) {
            perror("open");
            _exit(126);
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2");
            _exit(126);
        }
        if (fd > STDERR_FILENO)
            close(fd);
    }

    if (cfg.max_mem_mb > 0) {
        constexpr std::uint64_t kBytesPerMb = 1024ULL * 1024ULL;
        if (cfg.max_mem_mb > std::numeric_limits<rlim_t>::max() / kBytesPerMb) {
            constexpr char kMsg[] = "benchrun: memory limit overflows rlim_t\n";
            if (write(STDERR_FILENO, kMsg, sizeof(kMsg) - 1) < 0)
                perror("write");
            _exit(1);
        }

        const auto limit = static_cast<rlim_t>(cfg.max_mem_mb) * kBytesPerMb;
        const rlimit rl{limit, limit};
        if (setrlimit(RLIMIT_AS, &rl) != 0) {
            perror("setrlimit");
            _exit(1);
        }
    }

    if (cfg.quiet) {
        const int fd = open("/dev/null", O_WRONLY);
        if (fd < 0) {
            perror("open");
            _exit(1);
        }

        if (dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) {
            perror("dup2");
            _exit(1);
        }

        if (fd > STDERR_FILENO)
            close(fd);
    }
}

[[noreturn]] void exec_command(const PreparedCommand& cmd) {
    execvp(cmd.argv[0], cmd.argv.data());
    perror("execvp");
    _exit(127);
}

struct WaitResult {
    int status = 0;
    rusage usage{};
    bool timed_out = false;
    bool ok = false;
    int error = 0;
};

WaitResult wait_for_child(pid_t pid, std::uint64_t timeout_ms) {
    WaitResult result;
    if (timeout_ms == 0) {
        pid_t rc = 0;
        do {
            rc = wait4(pid, &result.status, 0, &result.usage);
        } while (rc < 0 && errno == EINTR);
        result.ok = rc == pid;
        if (!result.ok)
            result.error = errno;
        return result;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (true) {
        pid_t rc = wait4(pid, &result.status, WNOHANG, &result.usage);
        if (rc == pid) {
            result.ok = true;
            return result;
        }

        if (rc < 0) {
            if (errno == EINTR)
                continue;
            result.error = errno;
            return result;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            if (kill(-pid, SIGKILL) < 0 && errno != ESRCH) {
                result.error = errno;
                kill(pid, SIGKILL);
            }
            result.timed_out = true;
            do {
                rc = wait4(pid, &result.status, 0, &result.usage);
            } while (rc < 0 && errno == EINTR);
            result.ok = rc == pid;
            if (!result.ok && result.error == 0)
                result.error = errno;
            return result;
        }

        const auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        std::this_thread::sleep_for(std::min(remaining_ms, std::chrono::milliseconds(1)));
    }
}

RunExecution run_once(const Config& cfg, const PreparedCommand& cmd, std::size_t run_index, bool warmup) {
    std::fflush(nullptr);
    const pid_t pid = fork();
    if (pid < 0)
        return make_error("fork", errno);

    if (pid == 0) {
        if (setpgid(0, 0) != 0) {
            perror("setpgid");
            _exit(126);
        }
        setup_child_process(cfg);
        exec_command(cmd);
    }

    if (setpgid(pid, pid) != 0 && errno != EACCES && errno != ESRCH) {
        const int error = errno;
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        return make_error("setpgid", error);
    }

    const auto t0 = std::chrono::steady_clock::now();
    const WaitResult waitres = wait_for_child(pid, cfg.timeout_ms);
    const auto t1 = std::chrono::steady_clock::now();
    if (!waitres.ok)
        return make_error("wait4", waitres.error);

    RunResult result;
    result.run_index = run_index;
    result.warmup = warmup;
    result.start_time_ns = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t0.time_since_epoch()).count());
    result.end_time_ns = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1.time_since_epoch()).count());
    result.wall_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    result.user_ms = static_cast<double>(waitres.usage.ru_utime.tv_sec) * 1000.0 +
                     static_cast<double>(waitres.usage.ru_utime.tv_usec) / 1000.0;
    result.sys_ms = static_cast<double>(waitres.usage.ru_stime.tv_sec) * 1000.0 +
                    static_cast<double>(waitres.usage.ru_stime.tv_usec) / 1000.0;
    result.peak_rss_bytes = normalize_max_rss_byte(waitres.usage.ru_maxrss);
    if (waitres.timed_out) {
        result.outcome = RunOutcome::timed_out;
    } else if (WIFEXITED(waitres.status)) {
        result.exit_code = WEXITSTATUS(waitres.status);
        result.outcome = result.exit_code == 0 ? RunOutcome::succeeded : RunOutcome::exited_nonzero;
    } else if (WIFSIGNALED(waitres.status)) {
        result.term_signal = WTERMSIG(waitres.status);
        result.outcome = RunOutcome::signaled;
    }
    return result;
}

class PosixCommandExecutor final : public CommandExecutor {
public:
    explicit PosixCommandExecutor(const Config& cfg) : cfg_(cfg), command_(prepare_command(cfg)) {}

    RunExecution execute(std::size_t run_index, bool warmup) override {
        return run_once(cfg_, command_, run_index, warmup);
    }

private:
    const Config& cfg_;
    PreparedCommand command_;
};

bool succeeded(const RunResult& result) {
    return result.outcome == RunOutcome::succeeded;
}

} // namespace

BenchmarkExecution run_benchmark(const Config& cfg, CommandExecutor& executor) {
    BenchmarkExecution execution;
    if (cfg.command.empty()) {
        execution.error = RunnerError{"configuration", 0, "missing command"};
        return execution;
    }
    if (cfg.runs == 0) {
        execution.error = RunnerError{"configuration", 0, "runs must be greater than zero"};
        return execution;
    }

    execution.results.warmups.reserve(cfg.warmup);
    execution.results.measured.reserve(cfg.runs);
    for (std::size_t i = 0; i < cfg.warmup; ++i) {
        RunExecution run = executor.execute(i, true);
        if (const auto* error = std::get_if<RunnerError>(&run)) {
            execution.error = *error;
            return execution;
        }
        auto result = std::get<RunResult>(std::move(run));
        if (!succeeded(result)) {
            std::fprintf(stderr, "benchrun: warmup %zu did not succeed\n", i + 1);
        }
        execution.results.warmups.push_back(std::move(result));
    }

    for (std::size_t i = 0; i < cfg.runs; ++i) {
        RunExecution run = executor.execute(i, false);
        if (const auto* error = std::get_if<RunnerError>(&run)) {
            execution.error = *error;
            return execution;
        }
        execution.results.measured.push_back(std::get<RunResult>(std::move(run)));
    }
    return execution;
}

BenchmarkExecution run_benchmark(const Config& cfg) {
    PosixCommandExecutor executor(cfg);
    return run_benchmark(cfg, executor);
}
