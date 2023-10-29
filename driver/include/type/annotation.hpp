#pragma once

#include "util/util.hpp"
#include <iostream>
#include <unordered_map>

enum class AnnotationType { FREQ, CONSEC, COUNT };

static AnnotationType get_annotation_type_from_env(const char *env_var_name) {
    static const std::unordered_map<std::string, AnnotationType>
        annotation_type_map = {{"FREQ", AnnotationType::FREQ},
                               {"CONSEC", AnnotationType::CONSEC},
                               {"COUNT", AnnotationType::COUNT}};

    std::string env_value = get_env_value(env_var_name);

    auto it = annotation_type_map.find(env_value);
    if (it != annotation_type_map.end()) {
        return it->second;
    } else {
        std::cerr << "Invalid environment variable value for AnnotationType: "
                  << env_value << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
