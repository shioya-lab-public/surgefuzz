#pragma once

#include "method/blackbox.hpp"
#include "method/difuzzrtl.hpp"
#include "method/directfuzz.hpp"
#include "method/rfuzz.hpp"
#include "method/surgefuzz.hpp"
#include "surge/surge.hpp"
#include <fstream>
#include <iostream>
#include <memory>

class SurgeFuzzDriver {
  public:
    SurgeFuzzDriver() : surgefuzz(surge_recorder), surge_recorder() {}

    void call_per_one_cycle(auto *core) {
        surge_recorder.update(core->coverage_target);
        surgefuzz.update(core);
    }

  private:
    SurgeFuzz surgefuzz;
    SurgeRecorder surge_recorder;
};

template <typename T> class FuzzDriver {
  public:
    FuzzDriver() : method(), surge_recorder() {}

    void call_per_one_cycle(auto *core) {
        surge_recorder.update(core->coverage_target);
        method.update(core);
    }

  private:
    T method;
    SurgeRecorder surge_recorder;
};

class RegisterRecorder {
  public:
    RegisterRecorder(const std::string &log_filepath)
        : ofs(std::ofstream(log_filepath)),
          ancestore_count(get_size_from_env("FUZZ_ANCESTOR_COUNT")) {
        if (!this->ofs) {
            std::cout << "Cannot open the file for saving register."
                      << std::endl;
            exit(1);
        }
        this->ofs << "cycle,";
        for (std::size_t i = 0; i < ancestore_count; i++) {
            this->ofs << "dependent_" + std::to_string(i) << ",";
        }
        this->ofs << "coverage_target" << std::endl;
    };

    void call_per_one_cycle(std::uint32_t annotated_signal,
                            std::uint32_t *ancestore_register,
                            std::size_t cycle) {
        this->ofs << cycle << ",";
        for (std::size_t i = 0; i < ancestore_count; i++) {
            this->ofs << ancestore_register[i] << ",";
        }
        this->ofs << annotated_signal << std::endl;
    }

  private:
    std::ofstream ofs;
    std::size_t ancestore_count;
};
