#pragma once

#include "program.hpp"

#include <filesystem>
#include <optional>
#include <queue>
#include <random>
#include <vector>

class Seed;

class Corpus {
  public:
    Corpus(){};
    ~Corpus();

    // Disable copy constructors.
    Corpus(const Corpus &) = delete;
    Corpus &operator=(const Corpus &) = delete;

    Seed &get_next_seed(RandomNumberGenerator &rnd);
    void add_new_input(const Program &input, double energy, double score,
                       std::size_t cycle, std::optional<std::size_t> seed_id);
    std::size_t size() const;
    bool empty() const;
    void save(const std::filesystem::path &fuzz_dir) const;

  private:
    std::vector<Seed *> corpus;

#if defined(ENABLE_POWER_SCHEDULE)
    std::piecewise_constant_distribution<double> corpus_distribution;
    std::vector<double> intervals;
    std::vector<double> weights;
    void update_corpus_distribution();
#endif
};

class Seed {
  public:
    Seed(Program program, double energy, std::size_t id, std::size_t pid,
         double score, std::size_t cycle);

    void used();
    void save(const std::filesystem::path &save_dir) const;

    Program get_program() const;
    double get_energy() const;
    double get_init_energy() const;
    int get_used_count() const;
    size_t get_id() const;
    size_t get_pid() const;
    double get_score() const;
    size_t get_cycle() const;

  private:
    Program program;
    double energy;
    double init_energy;
    int used_count;
    std::size_t id;
    std::size_t pid;
    double score;
    std::size_t cycle;
};
