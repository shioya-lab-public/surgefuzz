#include "fuzz_driver.hpp"
#include "util.hpp"
#include <cstring>
#include <iostream>
#include <set>
#include <sys/shm.h>

struct CoreType {
    std::uint32_t coverage;
    std::uint32_t coverage_target;
};

void test_surgefuzz_consec() {
    constexpr std::size_t cycles = 20;
    const std::uint32_t coverage_signal_list[cycles] = {
        0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 0, 0, 1, 2, 3, 0, 1, 2, 5, 0};
    const std::uint32_t annotated_signal_list[cycles] = {
        0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0};

    constexpr std::size_t bitmap_size = 256;

    ShmInfo surge_score_bitmap_info =
        setup_shm(bitmap_size * sizeof(std::uint8_t));
    ShmInfo bitmap_info = setup_shm(bitmap_size);

    if (surge_score_bitmap_info.shm_addr == nullptr ||
        bitmap_info.shm_addr == nullptr) {
        std::cerr << "Failed to set up shared memory." << std::endl;
        return;
    }

    std::uint8_t *surge_score_bitmap =
        static_cast<std::uint8_t *>(surge_score_bitmap_info.shm_addr);
    std::uint8_t *bitmap = static_cast<std::uint8_t *>(bitmap_info.shm_addr);

    if ((setenv("FUZZ_METHOD_TYPE", "SurgeFuzz", 1) != 0) ||
        (setenv("FUZZ_ANNOTATION_TYPE", "CONSEC", 1) != 0) ||
        (setenv("FUZZ_SURGE_SCORE_BITMAP",
                std::to_string(surge_score_bitmap_info.shm_id).c_str(),
                1) != 0) ||
        (setenv("FUZZ_BITMAP", std::to_string(bitmap_info.shm_id).c_str(), 1) !=
         0)) {
        std::cerr << "Failed to set environment variables." << std::endl;
        cleanup_shm(surge_score_bitmap_info);
        cleanup_shm(bitmap_info);
        return;
    }

    SurgeFuzzDriver fuzz_driver;
    for (std::size_t i = 0; i < cycles; i++) {
        struct CoreType core = {coverage_signal_list[i],
                                annotated_signal_list[i]};
        fuzz_driver.call_per_one_cycle(&core);
    }

    std::cout << std::endl
              << "Starting SurgeFuzz(CONSEC) tests..." << std::endl;

    std::cout << "Checking the surge score bitmap..." << std::endl;
    for (std::size_t i = 0; i < bitmap_size; i++) {
        if (i <= 4) {
            assert(surge_score_bitmap[i] == 1);
        } else {
            assert(surge_score_bitmap[i] == 0);
        }
    }

    std::cout << "Checking the bitmap..." << std::endl;
    std::set<std::size_t> expected;
    for (std::size_t i = 0; i < cycles; i++) {
        std::uint32_t consec_score = 0;
        for (std::size_t j = 0; j <= i; j++) {
            consec_score = annotated_signal_list[j] ? consec_score + 1 : 0;
        }
        expected.insert((coverage_signal_list[i] << 4) | consec_score);
    }
    for (std::size_t i = 0; i < bitmap_size; i++) {
        if (expected.find(i) != expected.end()) {
            assert(bitmap[i] == 1);
        } else {
            assert(bitmap[i] == 0);
        }
    }

    std::cout << "All tests passed!" << std::endl << std::endl;

    unsetenv("FUZZ_METHOD_TYPE");
    unsetenv("FUZZ_ANNOTATION_TYPE");
    unsetenv("FUZZ_SURGE_SCORE_BITMAP");
    unsetenv("FUZZ_BITMAP");

    cleanup_shm(surge_score_bitmap_info);
    cleanup_shm(bitmap_info);
}
