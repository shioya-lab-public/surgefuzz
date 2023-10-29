#include "kernel/yosys.h"
#include "util.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct ParseAnnotationPass : public Pass {
    ParseAnnotationPass()
        : Pass("parse_annotation", "just a simple parser for annotations") {}

    void help() override {
        log("\n");
        log("\tparse_annotation [--output_annotation_type_filepath]\n");
        log("\n");
        log("This path parses the annotations and generates definitions to be "
            "used during fuzzing.\n");
        log("\n");
        log("\t--output_annotation_type_filepath filepath\n");
        log("\t\tfilepath to output parsed annotation type\n");
        log("\n");
    }

    std::string gen_annotation_type(RTLIL::Module *module) {
        validate_annotations(module);

        for (RTLIL::Wire *wire : module->selected_wires()) {
            if (wire->attributes.count(ID(SURGE_FREQ))) {
                return "FUZZ_ANNOTATION_TYPE: FREQ\n";
            }
            if (wire->attributes.count(ID(SURGE_CONSEC))) {
                return "FUZZ_ANNOTATION_TYPE: CONSEC\n";
            }
            if (wire->attributes.count(ID(SURGE_COUNT))) {
                return "FUZZ_ANNOTATION_TYPE: COUNT\n";
            }
        }
        log_assert(false);
        return "FUZZ_ANNOTATION_TYPE: UNKNOWN\n"; // unrechable
    }

    virtual void execute(std::vector<std::string> args,
                         RTLIL::Design *design) override {
        log_header(design, "Executing parse_annotation pass.\n");
        log_push();

        std::string output_annotation_type;
        for (size_t argidx = 1; argidx < args.size(); argidx++) {
            std::string arg = args[argidx];
            if (arg == "--output_annotation_type_filepath" &&
                argidx + 1 < args.size() && output_annotation_type.empty()) {
                output_annotation_type = args[++argidx];
            } else {
                log_error("Failed to parse an unkown arg: %s",
                          args[argidx].c_str());
            }
        }
        if (output_annotation_type.empty()) {
            help();
            log_cmd_error(
                "The option --output_annotation_type must be specified.");
        }

        for (auto module : design->selected_modules()) {
            std::ofstream fs(output_annotation_type);
            fs << gen_annotation_type(module);
        }

        log_pop();
    }
} ParseAnnotationPass;

PRIVATE_NAMESPACE_END
