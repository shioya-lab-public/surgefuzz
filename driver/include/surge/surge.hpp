#pragma once

#include "type/annotation.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

class SurgeRecorder {
  public:
    SurgeRecorder()
        : annotation_type(get_annotation_type_from_env("FUZZ_ANNOTATION_TYPE")),
          bitmap(get_shm_ptr<std::uint8_t>("FUZZ_SURGE_SCORE_BITMAP")),
          current_score(0) {}

    void update(std::uint32_t annotated_signal) {
        current_score = calc_surge_score(annotated_signal);
        bitmap[current_score & 0xff] = 1; // The bitmap size is 256.
    }

    std::uint32_t get_current_score() const { return current_score; }

  private:
    const AnnotationType annotation_type;
    uint8_t *const bitmap;
    std::uint32_t current_score;

    std::uint32_t calc_surge_score(std::uint32_t annotated_signal) {
        switch (annotation_type) {
        case AnnotationType::FREQ:
            return calc_freq_surge_score(annotated_signal);
        case AnnotationType::CONSEC:
            return calc_consec_surge_score(annotated_signal);
        case AnnotationType::COUNT:
            return calc_count_surge_score(annotated_signal);
        default:
            std::cerr << "Invalid annotation type." << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    std::uint32_t calc_freq_surge_score(std::uint32_t annotated_signal) {
        constexpr size_t HISTORY_SIZE = 256;

        static std::uint8_t history[HISTORY_SIZE] = {0};
        static std::uint32_t freq = 0;
        static std::uint32_t trace_count = 0;

        assert_valid_signal(annotated_signal);

        freq -= history[trace_count % HISTORY_SIZE];
        freq += annotated_signal;
        history[trace_count % HISTORY_SIZE] = annotated_signal;

        trace_count++;
        assert(freq <= HISTORY_SIZE);
        return freq;
    }

    std::uint32_t calc_consec_surge_score(std::uint32_t annotated_signal) {
        static std::uint32_t cur_seq = 0;
        assert_valid_signal(annotated_signal);

        return (annotated_signal == 1) ? ++cur_seq : (cur_seq = 0);
    }

    std::uint32_t calc_count_surge_score(std::uint32_t annotated_signal) const {
        return annotated_signal;
    }

    void assert_valid_signal(std::uint32_t signal) const {
        assert(signal == 0 || signal == 1);
    }
};
