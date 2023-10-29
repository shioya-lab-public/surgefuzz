#pragma once

struct Blackbox {
    Blackbox() {}
    virtual ~Blackbox() {}

    // Disable copy constructors.
    Blackbox(const Blackbox &) = delete;
    Blackbox &operator=(const Blackbox &) = delete;

    void update(auto *core){};
};
