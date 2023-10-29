#include "state.hpp"
#include "feedback.hpp"

#include <cassert>
#include <fstream>
#include <iostream>

State::State(const std::filesystem::path fuzz_dir)
    : current_cycle(0), total_crashes(0),
      crash_lines(std::map<std::size_t, std::size_t>()), total_timeouts(0),
      total_differential_bugs(0), achieved_coverage_rates(0.),
      achieved_target_coverage_count(0), achieved_best_score(0.),
      status_file(fopen((fuzz_dir / "status.csv").c_str(), "w"), &fclose),
      crash_lines_file(fopen((fuzz_dir / "crash_lines.csv").c_str(), "w"),
                       &fclose),
      fuzz_dir(fuzz_dir) {
    if (status_file.get() == nullptr) {
        std::cerr << "Cannot create " << fuzz_dir / "status.csv"
                  << " for saving status." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    fprintf(
        status_file.get(),
        "current_cycle,total_crashes,unique_crashes,total_differential_bugs,"
        "total_timeouts,achived_coverage_rates,achived_target_coverage_count,"
        "achived_best_score,score,corpus_size,elapsed_ms\n");

    if (crash_lines_file.get() == nullptr) {
        std::cerr << "Cannot create " << fuzz_dir / "crash_lines.json"
                  << " for saving crash lines." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    fprintf(crash_lines_file.get(), "{ ");

    // Create directories to save inputs.
    std::filesystem::create_directory(fuzz_dir / "corpus");
    std::filesystem::create_directory(fuzz_dir / "crashes");
    std::filesystem::create_directory(fuzz_dir / "differential_bugs");
}

void State::update(const std::filesystem::path &input_filepath, bool timeout,
                   bool differential_bug, const Feedback &feedback,
                   std::size_t corpus_size,
                   const std::optional<std::size_t> seed_id,
                   std::uint64_t elapsed_ms) {
    current_cycle++;

    std::optional<std::size_t> crash_line = feedback.get_crash_line();
    if (crash_line) {
        total_crashes++;
        crash_lines[crash_line.value()]++;
        save_input(input_filepath, fuzz_dir / "crashes", seed_id, elapsed_ms);
    }

    if (timeout) {
        total_timeouts++;
    }

    if (differential_bug) {
        total_differential_bugs++;
        save_input(input_filepath, fuzz_dir / "differential_bugs", seed_id,
                   elapsed_ms);
    }

    achieved_coverage_rates = feedback.coverage_rate;
    achieved_target_coverage_count = feedback.target_coverage_count;
    achieved_best_score = feedback.target_best_score;

    write_stats_to_csv(feedback.target_score, corpus_size, elapsed_ms);
    write_crash_lines_to_json();
}

void State::write_stats_to_csv(double score, std::size_t corpus_size,
                               std::uint64_t elapsed_ms) {
    fprintf(status_file.get(),
            "%lu,%lu,%lu,%lu,%lu,%.9lf,%lu,%.9lf,%.9lf,%lu,%lu\n",
            current_cycle, total_crashes, crash_lines.size(),
            total_differential_bugs, total_timeouts, achieved_coverage_rates,
            achieved_target_coverage_count, achieved_best_score, score,
            corpus_size, elapsed_ms);
}

void State::write_crash_lines_to_json() {
    if (current_cycle == 1) {
        fprintf(crash_lines_file.get(), " \"%lu\": {", current_cycle);
    } else {
        fprintf(crash_lines_file.get(), ", \"%lu\": {", current_cycle);
    }

    bool is_first = true;
    for (const auto &[line, count] : crash_lines) {
        if (!is_first) {
            fprintf(crash_lines_file.get(), ", ");
        }
        fprintf(crash_lines_file.get(), "\"%lu\": %lu", line, count);
        is_first = false;
    }

    fprintf(crash_lines_file.get(), "} ");
}

void State::save() {
    fprintf(crash_lines_file.get(), "}");
    execute_command(
        {"tar", "-zcvf", fuzz_dir / "corpus.tar.gz", "-C", fuzz_dir, "corpus"},
        true);
    execute_command({"tar", "-zcvf", fuzz_dir / "crashes.tar.gz", "-C",
                     fuzz_dir, "crashes"},
                    true);
    execute_command({"tar", "-zcvf", fuzz_dir / "differential_bugs.tar.gz",
                     "-C", fuzz_dir, "differential_bugs"},
                    true);
}

void State::save_input(const std::filesystem::path &input_filepath,
                       const std::filesystem::path &save_dir,
                       const std::optional<std::size_t> seed_id,
                       const std::uint64_t elapsed_ms) const {
    const std::string filename(
        "cycle:" + num2str(current_cycle, 6) + "," +
        "elapsed_ms:" + num2str(elapsed_ms, 6) + "," + "seed_id:" +
        (seed_id.has_value() ? num2str(seed_id.value(), 6) : "-1") + ".s");

    const std::filesystem::path save_filepath = save_dir / filename;
    std::filesystem::copy(input_filepath, save_filepath);
}
