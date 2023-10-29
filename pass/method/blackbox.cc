#include "kernel/celltypes.h"
#include "kernel/sigtools.h"
#include "kernel/yosys.h"

#include "util.h"
#include <vector>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct BlackboxPass : public Pass {
    BlackboxPass()
        : Pass("blackbox_cov_inst", "Instrument code for random generator") {}

    void help() override {
        log("\n");
        log("\tblackbox_cov_inst [--output_csv] [--output_dot] [--debug]\n");
        log("\n");
        log("This path instruments the code for random generator and outputs "
            "the instrumentation\n");
        log("information to a file.\n");
        log("\n");
        log("\t--output_csv filepath\n");
        log("\t\tfilepath to output the code instrumentation information\n");
        log("\n");
        log("\t--debug\n");
        log("\t\tenable debug log\n");
        log("\n");
    }

    void instrument(RTLIL::Module *module, const std::string &filepath) {
        std::ofstream fs(filepath);
        if (!fs) {
            log_error("Failed to open %s.", filepath.c_str());
        }
        // Output csv header
        fs << "coverage_signal_name,width" << std::endl;

        // Instrument target coverage signal
        RTLIL::SigSpec target_sig = get_annotated_signal(module);
        RTLIL::Wire *target_wire =
            module->addWire("\\coverage_target", target_sig.size());
        module->connect(target_wire, target_sig);
        fs << target_wire->name.substr(1) << "," << target_wire->width
           << std::endl;

        module->fixup_ports();
    }

    void execute(std::vector<std::string> args,
                 RTLIL::Design *design) override {
        log_header(design, "Executing random generator pass.\n");
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
            instrument(module, output_csv_filepath);
        }

        log_pop();
    }
} BlackboxPass;

PRIVATE_NAMESPACE_END
