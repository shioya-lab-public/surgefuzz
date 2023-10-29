#pragma once

#include "corpus.hpp"
#include "feedback.hpp"
#include "program.hpp"

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>

class State {
  public:
    State(const std::filesystem::path fuzz_dir);

    // Disable copy constructors.
    // This class has the reference to raw shared memories,
    // so the instance should be at most only one.
    State(const State &) = delete;
    State &operator=(const State &) = delete;

    void update(const std::filesystem::path &input_filepath, bool timeout,
                bool differential_bug, const Feedback &feedback,
                std::size_t corpus_size,
                const std::optional<std::size_t> seed_id,
                std::uint64_t elapsed_ms);
    void save();

  private:
    std::size_t current_cycle;
    std::size_t total_crashes;
    std::map<std::size_t, std::size_t> crash_lines; // line, count
    std::size_t total_timeouts;
    std::size_t total_differential_bugs;

    double achieved_coverage_rates;

    // Only used by directed fuzzer.
    std::size_t achieved_target_coverage_count;
    double achieved_best_score;

    std::unique_ptr<FILE, decltype(&fclose)> status_file;
    std::unique_ptr<FILE, decltype(&fclose)> crash_lines_file;

    const std::filesystem::path fuzz_dir;

    void write_stats_to_csv(double score, std::size_t corpus_size,
                            std::uint64_t elapsed_ms);
    void write_crash_lines_to_json();
    void save_input(const std::filesystem::path &input_filepath,
                    const std::filesystem::path &save_dir,
                    const std::optional<std::size_t> seed_id,
                    const std::uint64_t elapsed_ms) const;
};
