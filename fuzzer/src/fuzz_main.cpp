#include "config.hpp"
#include "fuzzer.hpp"
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <vector>

int main(int argc, char *argv[]) {
    Config::instance().parse_command_line_args(argc, argv);

    std::string fuzz_dir_str;
    std::string fuzz_input_patch;
    size_t fuzz_total_cycle = 0;
    size_t timeout_sec = 0;
    int fuzz_rnd_seed = 0;
    Mode mode = Mode::Invalid;
    std::string mode_str;
    std::string asm_header;
    std::string asm_footer;
    std::vector<std::string> gen_command;
    std::vector<std::string> build_command;
    std::vector<std::string> sim_command;

    for (int argidx = 1; argidx < argc; argidx++) {
        std::string arg = argv[argidx];
        if (arg == "--fuzz_dir" && argidx + 1 < argc) {
            fuzz_dir_str = argv[++argidx];
        } else if (arg == "--fuzz_input_patch" && argidx + 1 < argc) {
            fuzz_input_patch = argv[++argidx];
        } else if (arg == "--fuzz_total_cycle" && argidx + 1 < argc) {
            fuzz_total_cycle = std::stoi(argv[++argidx]);
        } else if (arg == "--timeout_sec" && argidx + 1 < argc) {
            timeout_sec = std::stoi(argv[++argidx]);
        } else if (arg == "--fuzz_rnd_seed" && argidx + 1 < argc) {
            fuzz_rnd_seed = std::stoi(argv[++argidx]);
        } else if (arg == "--fuzz_mode" && argidx + 1 < argc) {
            mode_str = argv[++argidx];
            if (mode_str == "surgefuzz") {
                mode = Mode::SurgeFuzz;
            } else if (mode_str == "rfuzz") {
                mode = Mode::RFuzz;
            } else if (mode_str == "difuzzrtl") {
                mode = Mode::DifuzzRTL;
            } else if (mode_str == "directfuzz") {
                mode = Mode::DirectFuzz;
            } else if (mode_str == "blackbox") {
                mode = Mode::Blackbox;
            } else {
                std::cerr << "Invalid mode: " << argv[argidx] << std::endl;
                std::exit(EXIT_FAILURE);
            }
        } else if (arg == "--fuzz_asm_header" && argidx + 1 < argc) {
            asm_header = argv[++argidx];
        } else if (arg == "--fuzz_asm_footer" && argidx + 1 < argc) {
            asm_footer = argv[++argidx];
        } else if (arg == "--fuzz_gen_command" && argidx + 1 < argc) {
            std::string gen_command_str = argv[++argidx];
            gen_command = split(gen_command_str);
        } else if (arg == "--fuzz_build_command" && argidx + 1 < argc) {
            std::string build_command_str = argv[++argidx];
            build_command = split(build_command_str);
        } else if (arg == "--fuzz_sim_command" && argidx + 1 < argc) {
            std::string sim_command_str = argv[++argidx];
            sim_command = split(sim_command_str);
        } else {
            // do nothing
        }
    }

    assert(not fuzz_dir_str.empty() && "--fuzz_dir error");
    assert((fuzz_total_cycle != 0 || timeout_sec != 0) &&
           "--fuzz_total_cycle/timeout_sec error");
    assert(mode != Mode::Invalid);

    const std::filesystem::path fuzz_dir(fuzz_dir_str);
    const std::filesystem::path path_to_input(fuzz_input_patch);

    std::cout << "Start fuzzing" << std::endl;

    auto fuzzer =
        create_fuzzer(mode, fuzz_dir, fuzz_rnd_seed, gen_command, build_command,
                      sim_command, path_to_input, asm_header, asm_footer);
    fuzzer->fuzz_loop(fuzz_total_cycle, timeout_sec);

    std::cout << "End fuzzing" << std::endl;
    return 0;
}
