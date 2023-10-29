#pragma once

#include "surge/surge.hpp"
#include "util/util.hpp"
#include <cstdint>
#include <iostream>

class SurgeFuzz {
  public:
    SurgeFuzz(const SurgeRecorder &surge_recorder)
        : bitmap(get_shm_ptr<std::uint8_t>("FUZZ_BITMAP")),
          surge_recorder(surge_recorder),
          annotation_type(
              get_annotation_type_from_env("FUZZ_ANNOTATION_TYPE")) {}

    void update(auto *core) {
        switch (annotation_type) {
        case AnnotationType::FREQ:
        case AnnotationType::CONSEC:
            return update_bitmap(core->coverage,
                                 surge_recorder.get_current_score());
        case AnnotationType::COUNT:
            return update_bitmap(core->coverage);
        default:
            std::cerr << "Invalid annotation type." << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

  private:
    std::uint8_t *const bitmap;
    const SurgeRecorder &surge_recorder;
    const AnnotationType annotation_type;

    void update_bitmap(std::uint32_t coverage_signal) {
        bitmap[coverage_signal] = 1;
    }

    void update_bitmap(std::uint32_t coverage_signal,
                       std::uint32_t surge_score) {
        bitmap[(coverage_signal << 4) | (surge_score & 0xf)] = 1;
    }
};
