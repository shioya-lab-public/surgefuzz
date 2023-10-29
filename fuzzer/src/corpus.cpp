#include "corpus.hpp"
#include "config.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <type_traits>

Corpus::~Corpus() {
    for (Seed *seed : corpus) {
        delete seed;
    }
}

Seed &Corpus::get_next_seed(RandomNumberGenerator &rnd) {
    assert(not corpus.empty());

#if defined(ENABLE_POWER_SCHEDULE)
    update_corpus_distribution();
    std::size_t idx = static_cast<size_t>(corpus_distribution(rnd.mt));
#else
    std::size_t idx = rnd(corpus.size());
#endif
    assert(idx < corpus.size());

    Seed &seed = *corpus[idx];
    return seed;
}

void Corpus::add_new_input(const Program &input, double energy, double score,
                           std::size_t cycle,
                           std::optional<std::size_t> seed_id) {
    if (seed_id.has_value()) {
        // Add a new input to seed inputs.
        corpus.emplace_back(new Seed(input, energy, corpus.size(),
                                     seed_id.value(), score, cycle));
    } else {
        // Add an initail input to seed inputs.
        assert(corpus.size() < Config::instance().initial_seed_count);
        corpus.emplace_back(
            new Seed(input, energy, 0, 0 /* dummy value */, score, cycle));
    }
}

std::size_t Corpus::size() const { return corpus.size(); }

bool Corpus::empty() const { return corpus.empty(); }

void Corpus::save(const std::filesystem::path &fuzz_dir) const {
    std::filesystem::create_directory(fuzz_dir / "corpus");
    for (const Seed *seed : corpus) {
        seed->save(fuzz_dir / "corpus");
    }
}

#if defined(ENABLE_POWER_SCHEDULE)
void Corpus::update_corpus_distribution() {
    std::size_t corpus_size = corpus.size();
    assert(corpus_size);

    intervals.resize(corpus_size + 1);
    std::iota(intervals.begin(), intervals.end(), 0);

    weights.resize(corpus_size);
    for (std::size_t i = 0; i < corpus_size; i++) {
        weights[i] = corpus[i]->get_energy() + 1e-6;
    }
    corpus_distribution = std::piecewise_constant_distribution<double>(
        intervals.begin(), intervals.end(), weights.begin());
}
#endif

Seed::Seed(Program program, double energy, std::size_t id, std::size_t pid,
           double score, std::size_t cycle)
    : program(program), energy(energy), init_energy(energy), used_count(0),
      id(id), pid(pid), score(score), cycle(cycle) {}

void Seed::used() { used_count++; }

void Seed::save(const std::filesystem::path &save_dir) const {
    const std::string filename =
        "id:" + num2str(get_id(), 6) + "," + "pid:" + num2str(get_pid(), 6) +
        "," + "cycle:" + num2str(get_cycle(), 6) + "," +
        "score:" + std::to_string(get_score()) + "," +
        "energy:" + std::to_string(get_energy()) + "," +
        "init_energy:" + std::to_string(get_init_energy()) + "," +
        "used_count:" + num2str(get_used_count(), 5) + ".s";
    const std::filesystem::path filepath = save_dir / filename;
    std::ofstream ofs(filepath);
    if (!ofs) {
        std::cerr << "Cannot open the file for saving a input." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    ofs << get_program().generate() << std::endl;
    ofs.close();
}

Program Seed::get_program() const { return program; }
double Seed::get_energy() const { return energy; }
double Seed::get_init_energy() const { return init_energy; }
int Seed::get_used_count() const { return used_count; }
size_t Seed::get_id() const { return id; }
size_t Seed::get_pid() const { return pid; }
double Seed::get_score() const { return score; }
size_t Seed::get_cycle() const { return cycle; }
