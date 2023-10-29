ANNOTATION := ldq_full
# ANNOTATION := store_blocked

CHIPYARD := /chipyard

FUZZ_INPUT_FILEPATH := $(WORKDIR)/fuzz.S

FUZZ_GEN_COMMAND := ""

FUZZ_BUILD_COMMAND := "${CHIPYARD}/.conda-env/riscv-tools/bin/riscv64-unknown-elf-gcc \
    -march=rv64g -mabi=lp64 \
    -static -mcmodel=medany \
    -fvisibility=hidden -nostdlib -nostartfiles \
    -I${CHIPYARD}/toolchains/riscv-tools/riscv-tests/env/p \
    -I${CHIPYARD}/toolchains/riscv-tools/riscv-tests/isa/macros/scalar \
    -T${CHIPYARD}/toolchains/riscv-tools/riscv-tests/env/p/link.ld \
    ${WORKDIR}/fuzz.S -o ${WORKDIR}/fuzz"

FUZZ_SIM_COMMAND := "${CHIPYARD}/sims/verilator/simulator-chipyard-SmallBoomConfig \
    --cycle-count \
    --max-cycles=15000 \
    --seed=0 \
    ${WORKDIR}/fuzz"

define FUZZ_ASM_HEADER
#include "riscv_test.h"
#include "test_macros.h"

RVTEST_RV64U
RVTEST_CODE_BEGIN
  j test_start
test_start:
  .rept 0x8
  nop
  .endr
  fence
  .rept 0x8
  nop
  .endr

endef

define FUZZ_ASM_FOOTER
  RVTEST_PASS
RVTEST_CODE_END
  .data
RVTEST_DATA_BEGIN
  TEST_DATA
test_memory:
RVTEST_DATA_END
endef
