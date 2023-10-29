#ifndef ANNOTATION_H
#define ANNOTATION_H

#include "kernel/sigtools.h"
#include "kernel/yosys.h"

YOSYS_NAMESPACE_BEGIN

bool is_annotated_wire(RTLIL::Wire *wire) {
    if (wire->attributes.count(ID(SURGE_FREQ)) ||
        wire->attributes.count(ID(SURGE_CONSEC)) ||
        wire->attributes.count(ID(SURGE_COUNT))) {
        return true;
    } else {
        return false;
    }
}

void validate_annotations(RTLIL::Module *module) {
    std::size_t annotated_wire_cnt = 0;
    for (RTLIL::Wire *wire : module->selected_wires()) {
        if (is_annotated_wire(wire)) {
            log("Found wire %s with annotation.\n", log_signal(wire));
            annotated_wire_cnt++;
        }
    }
    if (annotated_wire_cnt == 0) {
        log_error(
            "Failed to find SURGE_FREQ/SURGE_CONSEC/SURGE_COUNT attribute.\n");
    }
    if (annotated_wire_cnt != 1) {
        log_error("Only one annotation is supported.\n");
    }
}

RTLIL::SigSpec get_annotated_signal(RTLIL::Module *module) {
    validate_annotations(module);

    for (RTLIL::Wire *wire : module->selected_wires()) {
        if (is_annotated_wire(wire)) {
            log("Found wire %s with annotation.\n", log_signal(wire));
            SigMap sigmap(module);
            SigSpec target_sig = sigmap(wire);
            log("Found target_sig %s after sigmap.\n", log_signal(target_sig));
            return target_sig;
        }
    }
    log_error("Unreachable statement.\n");
}

YOSYS_NAMESPACE_END

#endif
