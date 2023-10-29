#include "fuzzer.hpp"
#include "config.hpp"
#include "coverage/blackbox.hpp"
#include "coverage/difuzzrtl.hpp"
#include "coverage/directfuzz.hpp"
#include "coverage/rfuzz.hpp"
#include "coverage/surgefuzz.hpp"
#include <array>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <vector>

template <class Coverage>
Fuzzer<Coverage>::Fuzzer(const std::filesystem::path &fuzz_dir, int rnd_seed,
                         const std::vector<std::string> &gen_argv,
                         const std::vector<std::string> &build_argv,
                         const std::vector<std::string> &sim_argv,
                         const std::filesystem::path &path_to_input,
                         const std::string &header, const std::string &footer)
    : fuzz_dir(fuzz_dir), rnd_seed(rnd_seed),
      rnd(RandomNumberGenerator(rnd_seed)),
      executor(Executor<Coverage>(sim_argv, true)), state(State(fuzz_dir)),
      corpus(Corpus()), gen_argv(gen_argv), build_argv(build_argv),
      path_to_input(path_to_input), header(header), footer(footer) {
    std::filesystem::create_directory(fuzz_dir);
};

template <class Coverage>
void Fuzzer<Coverage>::fuzz_loop(std::size_t fuzz_total_cycle,
                                 std::uint64_t timeout_sec) {
    std::size_t fuzz_cycle = 0;
    auto start = std::chrono::steady_clock::now();
    while (true) {
        fuzz_cycle++;
        auto elapsed_ms = since(start).count();

        // condition of termination
        if (fuzz_total_cycle != 0 && fuzz_cycle > fuzz_total_cycle) {
            break;
        }
        if (timeout_sec != 0 &&
            static_cast<uint64_t>(elapsed_ms) > timeout_sec * 1000) {
            break;
        }

        fuzz_one(fuzz_cycle, elapsed_ms);
    }
    save_config(fuzz_total_cycle);
    corpus.save(fuzz_dir);
    state.save();
}

template <class Coverage>
void Fuzzer<Coverage>::fuzz_one(std::size_t fuzz_cycle,
                                std::uint64_t elapsed_ms) {
    // Generate DUT's input.
#ifdef ENABLE_DEBUG_PRINT
    std::cout << "1. Generate an input." << std::endl;
#endif
    Program input;
    std::optional<std::size_t> seed_id;
    {
        if (corpus.size() >= Config::instance().initial_seed_count) {
            Seed &seed = corpus.get_next_seed(rnd);
            input = seed.get_program();
            input.mutate(rnd);
            seed_id = seed.get_id();

            // Add used counter
            seed.used();
        } else {
            input = Program(
                Config::instance().initial_seed_block_count,
                Config::instance().initial_seed_instructions_per_block, rnd);
            seed_id = std::nullopt;
        }
    }
#ifdef ENABLE_DEBUG_PRINT
    std::cout << input.generate() << std::endl;
#endif
    input.write_to_file(path_to_input, header, footer);

    // Build an input by a compiler.
#ifdef ENABLE_DEBUG_PRINT
    std::cout << "2. Build a fuzz input." << std::endl;
    int build_exist_code = execute_command(build_argv, false);
#else
    int build_exist_code = execute_command(build_argv, true);
#endif

    if (build_exist_code != 0) {
        std::cerr << "Failed to build a generated code." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Executes a target design on RTL simulator(verilator), and
    // Monitors runtime status and generates feedback.
#ifdef ENABLE_DEBUG_PRINT
    std::cout << "3. Run DUT on RTL simulator." << std::endl;
#endif
    const Feedback feedback = executor.run();
    std::cout << std::right << "[fuzz_cycle: " << std::setw(5) << fuzz_cycle
              << "],  "
              << "[current surge score: " << std::setw(3)
              << feedback.target_score << "],  "
              << "[max surge score: " << std::setw(3)
              << feedback.target_best_score << "]" << std::endl;
#ifdef ENABLE_DEBUG_PRINT
    std::cout << feedback.to_string() << std::endl;
#endif

    // Add the new input to corpus when the input achieves new coverage,
    // or when this input is generated for a initial seed.
    if (feedback.new_coverage || seed_id == std::nullopt) {
#ifdef ENABLE_DEBUG_PRINT
        std::cout << "4. Add a new input to corpus." << std::endl;
#endif
        corpus.add_new_input(input, feedback.energy, feedback.target_score,
                             fuzz_cycle, seed_id);
    }

    state.update(path_to_input, false /* TODO: timeout */,
                 false /* TODO: differential bug */, feedback, corpus.size(),
                 seed_id, elapsed_ms);
}

template <class Coverage>
void Fuzzer<Coverage>::save_config(std::size_t fuzz_total_cycle) {
    std::filesystem::path filepath = fuzz_dir / "config.out";
    std::ofstream ofs(filepath, std::ios::out | std::ios::trunc);
    if (!ofs) {
        std::cout << "Cannot open a file for saving results." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    ofs << "fuzz_total_cycle: " << fuzz_total_cycle << std::endl;
    ofs << "rnd_seed: " << rnd_seed << std::endl;
    ofs.close();
}

template class Fuzzer<SurgeFuzz>;
template class Fuzzer<RFuzz>;
template class Fuzzer<DifuzzRTL>;
template class Fuzzer<DirectFuzz>;
template class Fuzzer<Blackbox>;

std::unique_ptr<BaseFuzzer>
create_fuzzer(Mode mode, const std::filesystem::path &fuzz_dir, int rnd_seed,
              const std::vector<std::string> &gen_argv,
              const std::vector<std::string> &build_argv,
              const std::vector<std::string> &sim_argv,
              const std::filesystem::path &path_to_input,
              const std::string &header, const std::string &footer) {
    switch (mode) {
    case Mode::SurgeFuzz:
        return std::make_unique<Fuzzer<SurgeFuzz>>(
            fuzz_dir, rnd_seed, gen_argv, build_argv, sim_argv, path_to_input,
            header, footer);
    case Mode::RFuzz:
        return std::make_unique<Fuzzer<RFuzz>>(fuzz_dir, rnd_seed, gen_argv,
                                               build_argv, sim_argv,
                                               path_to_input, header, footer);
    case Mode::DifuzzRTL:
        return std::make_unique<Fuzzer<DifuzzRTL>>(
            fuzz_dir, rnd_seed, gen_argv, build_argv, sim_argv, path_to_input,
            header, footer);
    case Mode::DirectFuzz:
        return std::make_unique<Fuzzer<DirectFuzz>>(
            fuzz_dir, rnd_seed, gen_argv, build_argv, sim_argv, path_to_input,
            header, footer);
    case Mode::Blackbox:
        return std::make_unique<Fuzzer<Blackbox>>(
            fuzz_dir, rnd_seed, gen_argv, build_argv, sim_argv, path_to_input,
            header, footer);
    default:
        std::cerr << "Invalid mode." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
