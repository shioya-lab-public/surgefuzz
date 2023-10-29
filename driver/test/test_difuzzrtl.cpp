#include "fuzz_driver.hpp"
#include "util.hpp"
#include <cstring>
#include <iostream>
#include <set>
#include <sys/shm.h>

struct CoreType {
    std::uint32_t *coverage;
    std::uint32_t coverage_target;
};

void test_difuzzrtl() {
    constexpr std::size_t cycles = 20;
    constexpr std::size_t instance_count = 4;
    std::uint32_t coverage_signal_list[cycles][instance_count] = {
        {0, 0, 0, 0}, {1, 0, 0, 0}, {0, 2, 0, 0}, {1, 3, 0, 0}, {1, 4, 0, 0},
        {1, 5, 1, 0}, {0, 6, 3, 0}, {0, 7, 0, 0}, {1, 7, 0, 0}, {1, 3, 0, 1},
        {1, 7, 0, 0}, {1, 3, 0, 0}, {0, 7, 7, 0}, {0, 6, 1, 0}, {0, 5, 0, 5},
        {0, 4, 0, 0}, {0, 3, 0, 0}, {1, 2, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 7}};
    const std::uint32_t annotated_signal_list[cycles] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    constexpr std::size_t surge_score_bitmap_size = 256;
    constexpr std::size_t bitmap_byte_size = 8;

    ShmInfo surge_score_bitmap_info =
        setup_shm(surge_score_bitmap_size * sizeof(std::uint64_t));
    ShmInfo bitmap_info =
        setup_shm(bitmap_byte_size * instance_count * sizeof(std::uint8_t));

    if (surge_score_bitmap_info.shm_addr == nullptr ||
        bitmap_info.shm_addr == nullptr) {
        std::cerr << "Failed to set up shared memory." << std::endl;
        return;
    }

    std::uint8_t *surge_score_bitmap =
        static_cast<std::uint8_t *>(surge_score_bitmap_info.shm_addr);
    std::uint8_t *bitmap = static_cast<std::uint8_t *>(bitmap_info.shm_addr);

    if ((setenv("FUZZ_METHOD_TYPE", "DifuzzRTL", 1) != 0) ||
        (setenv("FUZZ_ANNOTATION_TYPE", "COUNT", 1) != 0) ||
        (setenv("FUZZ_SURGE_SCORE_BITMAP",
                std::to_string(surge_score_bitmap_info.shm_id).c_str(),
                1) != 0) ||
        (setenv("FUZZ_BITMAP", std::to_string(bitmap_info.shm_id).c_str(), 1) !=
         0) ||
        (setenv("FUZZ_BITMAP_BYTE_SIZE",
                std::to_string(bitmap_byte_size).c_str(), 1) != 0) ||
        (setenv("FUZZ_BITMAP_INSTANCE_COUNT",
                std::to_string(instance_count).c_str(), 1) != 0)) {
        std::cerr << "Failed to set environment variables." << std::endl;
        cleanup_shm(surge_score_bitmap_info);
        cleanup_shm(bitmap_info);
        return;
    }

    FuzzDriver<DifuzzRTL> fuzz_driver;
    for (std::size_t i = 0; i < cycles; i++) {
        struct CoreType core = {&coverage_signal_list[i][0],
                                annotated_signal_list[i]};
        fuzz_driver.call_per_one_cycle(&core);
    }

    std::cout << std::endl << "Starting DifuzzRTL tests..." << std::endl;

    std::cout << "Checking the surge score bitmap..." << std::endl;
    for (std::size_t i = 0; i < surge_score_bitmap_size; i++) {
        if (i <= 9 || i == 16) {
            assert(surge_score_bitmap[i] == 1);
        } else {
            assert(surge_score_bitmap[i] == 0);
        }
    }

    std::cout << "Checking the bitmap..." << std::endl;
    std::set<std::size_t> expceted = {0,
                                      1,
                                      bitmap_byte_size + 0,
                                      bitmap_byte_size + 1,
                                      bitmap_byte_size + 2,
                                      bitmap_byte_size + 3,
                                      bitmap_byte_size + 4,
                                      bitmap_byte_size + 5,
                                      bitmap_byte_size + 6,
                                      bitmap_byte_size + 7,
                                      bitmap_byte_size * 2 + 0,
                                      bitmap_byte_size * 2 + 1,
                                      bitmap_byte_size * 2 + 3,
                                      bitmap_byte_size * 2 + 7,
                                      bitmap_byte_size * 3 + 0,
                                      bitmap_byte_size * 3 + 1,
                                      bitmap_byte_size * 3 + 5,
                                      bitmap_byte_size * 3 + 7};
    for (std::size_t i = 0; i < bitmap_byte_size * instance_count; i++) {
        if (expceted.find(i) != expceted.end()) {
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
