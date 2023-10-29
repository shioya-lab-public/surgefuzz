#pragma once

class CoverageBase {
  public:
    CoverageBase() {}

    // Disable copy constructors.
    CoverageBase(const CoverageBase &) = delete;
    CoverageBase &operator=(const CoverageBase &) = delete;

    virtual double coverage_rate() const = 0;
    virtual void reset() = 0;
    virtual bool new_coverage() const = 0;
    virtual void apply() = 0;
    virtual double energy(uint64_t) const { return 0; }
};
