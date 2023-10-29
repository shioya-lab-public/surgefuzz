#pragma once

#include "corpus.hpp"
#include "coverage/difuzzrtl.hpp"
#include "coverage/rfuzz.hpp"
#include "coverage/surgefuzz.hpp"
#include "executor.hpp"
#include "monitor.hpp"
#include "program.hpp"
#include "state.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <set>
#include <vector>

class BaseFuzzer {
  public:
    virtual ~BaseFuzzer() {}
    virtual void fuzz_loop(std::size_t fuzz_total_cycle,
                           std::uint64_t timeout_sec) = 0;
};

template <class Coverage> class Fuzzer : public BaseFuzzer {
  public:
    Fuzzer(const std::filesystem::path &fuzz_dir, int rnd_seed,
           const std::vector<std::string> &gen_argv,
           const std::vector<std::string> &build_argv,
           const std::vector<std::string> &sim_argv,
           const std::filesystem::path &path_to_input,
           const std::string &header, const std::string &footer);
    ~Fuzzer() {}

    void fuzz_loop(std::size_t fuzz_total_cycle,
                   std::uint64_t timeout_sec) override;
    void fuzz_one(std::size_t fuzz_cycle, std::uint64_t elapsed_ms);

  private:
    const std::filesystem::path fuzz_dir;

    const int rnd_seed;
    RandomNumberGenerator rnd;

    Executor<Coverage> executor;
    State state;
    Corpus corpus;

    const std::vector<std::string> gen_argv;
    const std::vector<std::string> build_argv;
    const std::filesystem::path path_to_input;
    const std::string header;
    const std::string footer;

    void save_config(std::size_t fuzz_total_cycle);
};

std::unique_ptr<BaseFuzzer>
create_fuzzer(Mode mode, const std::filesystem::path &fuzz_dir, int rnd_seed,
              const std::vector<std::string> &gen_argv,
              const std::vector<std::string> &build_argv,
              const std::vector<std::string> &sim_argv,
              const std::filesystem::path &path_to_input,
              const std::string &header, const std::string &footer);
