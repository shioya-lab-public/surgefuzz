#include "fuzz_driver.hpp"
#include "util.hpp"
#include <cstring>
#include <iostream>
#include <sys/shm.h>

void test_directfuzz() {
    constexpr std::size_t cycles = 20;
    constexpr std::size_t coverage_signal_width = 64;
    constexpr std::size_t instance_count = 2;

    struct CoreType {
        std::uint32_t (*coverage)[coverage_signal_width / 32]; // 2D array
        std::uint32_t coverage_target;
    };

    std::uint32_t
        coverage_signal_list[cycles][instance_count][coverage_signal_width /
                                                     32] = {
            {{0b0000'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1111}},
            {{0b1000'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1101}},
            {{0b0000'0000, 0b0100'0100}, {0b0000'0000, 0b0000'1101}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1111}},
            {{0b0001'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1111}},
            {{0b0001'0000, 0b0000'0000}, {0b0000'0000, 0b0000'1011}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0001, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0010}, {0b0000'0001, 0b0000'1111}},
            {{0b1000'0000, 0b0000'0000}, {0b0000'0001, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0001, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0001, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0001, 0b0000'1111}},
            {{0b0000'0000, 0b0000'0000}, {0b0000'0001, 0b0000'1111}}};

    const std::uint32_t annotated_signal_list[cycles] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    constexpr std::size_t surge_score_bitmap_size = 256;
    constexpr std::size_t bitmap_byte_size =
        coverage_signal_width / 8 * instance_count;

    ShmInfo surge_score_bitmap_info =
        setup_shm(surge_score_bitmap_size * sizeof(std::uint64_t));
    ShmInfo bitmap_info =
        setup_shm(bitmap_byte_size / 4 * sizeof(std::uint32_t));

    if (surge_score_bitmap_info.shm_addr == nullptr ||
        bitmap_info.shm_addr == nullptr) {
        std::cerr << "Failed to set up shared memory." << std::endl;
        return;
    }

    std::uint8_t *surge_score_bitmap =
        static_cast<std::uint8_t *>(surge_score_bitmap_info.shm_addr);
    std::uint32_t(*bitmap)[coverage_signal_width / 32] =
        reinterpret_cast<std::uint32_t(*)[coverage_signal_width / 32]>(
            bitmap_info.shm_addr);

    if ((setenv("FUZZ_METHOD_TYPE", "DirectFuzz", 1) != 0) ||
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

    FuzzDriver<DirectFuzz> fuzz_driver;
    for (std::size_t i = 0; i < cycles; i++) {
        struct CoreType core = {coverage_signal_list[i],
                                annotated_signal_list[i]};
        fuzz_driver.call_per_one_cycle(&core);
    }

    std::cout << std::endl << "Starting DirectFuzz tests..." << std::endl;

    std::cout << "Checking the surge score bitmap..." << std::endl;
    for (std::size_t i = 0; i < surge_score_bitmap_size; i++) {
        if (i <= 9 || i == 16) {
            assert(surge_score_bitmap[i] == 1);
        } else {
            assert(surge_score_bitmap[i] == 0);
        }
    }

    std::cout << "Checking the bitmap..." << std::endl;
    std::uint32_t expected[][coverage_signal_width / 32] = {
        {0b1001'0000, 0b0100'0110}, {0b0000'0001, 0b0000'0110}};
    for (unsigned int instance_idx = 0; instance_idx < instance_count;
         instance_idx++) {
        for (std::size_t chunk_idx = 0; chunk_idx < coverage_signal_width / 32;
             chunk_idx++) {
            // printf("bitmap[%lu][%lu] = %lx\n", instance_idx, chunk_idx,
            //        bitmap[instance_idx][chunk_idx]);
            assert(bitmap[instance_idx][chunk_idx] ==
                   expected[instance_idx][chunk_idx]);
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
