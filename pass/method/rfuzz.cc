#include "kernel/celltypes.h"
#include "kernel/sigtools.h"
#include "kernel/yosys.h"

#include "util.h"
#include <vector>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

class MuxCoverage {
  public:
    MuxCoverage() {
        ct_mux.setup_type(ID($mux), {ID::A, ID::B, ID::S}, {ID::Y}, true);
        ct_pmux.setup_type(ID($pmux), {ID::A, ID::B, ID::S}, {ID::Y}, true);
    }

    void analyze(RTLIL::Module *module) {
        SigMap sigmap(module);
        for (auto cell : module->cells()) {
            if (ct_pmux.cell_known(cell->type)) {
                log_error("Found $pmux cell\n");
            }
            if (ct_mux.cell_known(cell->type)) {
                const RTLIL::SigSpec mux_select_signal =
                    sigmap(cell->getPort(ID::S));
                add(mux_select_signal);
            }
        }
    }

    void instrument(RTLIL::Module *module, const std::string &filepath) {
        std::ofstream fs(filepath);
        if (!fs) {
            log_error("Failed to open %s.", filepath.c_str());
        }
        // Output csv header
        fs << "coverage_signal_name,width" << std::endl;

        // Instrument coverage signal
        RTLIL::SigSpec coverage_sig;
        {
            for (auto sig : unique_mux_select_signals) {
                coverage_sig.append(sig);
            }
        }
        RTLIL::Wire *coverage_wire =
            module->addWire("\\coverage", coverage_sig.size());
        module->connect(coverage_wire, coverage_sig);
        fs << coverage_wire->name.substr(1) << "," << coverage_wire->width
           << std::endl;

        // Instrument target coverage signal
        RTLIL::SigSpec target_sig = get_annotated_signal(module);
        RTLIL::Wire *target_wire =
            module->addWire("\\coverage_target", target_sig.size());
        module->connect(target_wire, target_sig);
        fs << target_wire->name.substr(1) << "," << target_wire->width
           << std::endl;

        module->fixup_ports();
    }

    void debug() {
        log("---- Mux Select Signals ----\n");
        for (const RTLIL::SigSpec &sig : unique_mux_select_signals) {
            log("%s\n", log_signal(sig));
        }
        log("[Result] unique mux: %ld, total mux: %ld\n",
            unique_mux_select_signals.size(), mux_select_signals.size());
    }

    pool<RTLIL::SigSpec> unique_mux_select_signals;
    std::vector<RTLIL::SigSpec> mux_select_signals;

  private:
    CellTypes ct_mux, ct_pmux;

    void add(const RTLIL::SigSpec &sig) {
        log_assert(sig.size() == 1);
        unique_mux_select_signals.insert(sig);
        mux_select_signals.emplace_back(sig);
    }
};

struct RFuzzPass : public Pass {
    RFuzzPass() : Pass("rfuzz_cov_inst", "Instrument code for RFUZZ") {}

    void help() override {
        log("\n");
        log("\trfuzz_cov_inst [--output_csv] [--output_dot] [--debug]\n");
        log("\n");
        log("This path instruments the code for RFUZZ and outputs the "
            "instrumentation\n");
        log("information to a file. The path can also output an instance "
            "ceconnectivity graph.\n");
        log("\n");
        log("\t--output_csv filepath\n");
        log("\t\tfilepath to output the code instrumentation information\n");
        log("\n");
        log("\t--debug\n");
        log("\t\tenable debug log\n");
        log("\n");
    }

    void execute(std::vector<std::string> args,
                 RTLIL::Design *design) override {
        log_header(design, "Executing rfuzz pass (instrument directed mux "
                           "toggle coverage).\n");
        log_push();

        std::string output_csv_filepath;
        bool debug = false;
        for (size_t argidx = 1; argidx < args.size(); argidx++) {
            std::string arg = args[argidx];
            if (arg == "--output_csv" && argidx + 1 < args.size() &&
                output_csv_filepath.empty()) {
                output_csv_filepath = args[++argidx];
            } else if (arg == "--debug") {
                debug = true;
            } else {
                log_error("Failed to parse an unkown arg: %s\n",
                          args[argidx].c_str());
            }
        }
        if (output_csv_filepath.empty()) {
            help();
            log_cmd_error("The option --output_csv must be specified.");
        }

        for (auto module : design->selected_modules()) {
            // Analyze mux select signals.
            MuxCoverage coverage;
            coverage.analyze(module);
            if (debug) {
                coverage.debug();
            }
            coverage.instrument(module, output_csv_filepath);
        }

        log_pop();
    }
} RFuzzPass;

PRIVATE_NAMESPACE_END
