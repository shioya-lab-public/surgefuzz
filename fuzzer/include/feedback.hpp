#pragma once

#include <optional>
#include <string>
#include <vector>

class Feedback {
  public:
    Feedback(const bool is_success,
             const std::vector<std::uint8_t> &stdout_buffer,
             const std::vector<std::uint8_t> &stderr_buffer, bool new_coverage,
             double coverage_rate, std::size_t target_coverage_count,
             double target_best_socre, double target_score, double energy);

    // Disable copy constructors.
    // This class has the reference to raw shared memories,
    // so the instance should be at most only one.
    Feedback(const Feedback &) = delete;
    Feedback &operator=(const Feedback &) = delete;

    std::string get_stdout_str() const;
    std::string get_stderr_str() const;
    std::string to_string() const;
    std::optional<std::size_t> get_crash_line() const;

    const bool is_success;
    const std::vector<std::uint8_t> &stdout_buffer;
    const std::vector<std::uint8_t> &stderr_buffer;
    const bool new_coverage;
    const double coverage_rate;
    const std::size_t target_coverage_count;
    const double target_best_score;
    const double target_score;
    const double energy;
};
