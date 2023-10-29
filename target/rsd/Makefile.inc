ANNOTATION := "load_queue_max"
# ANNOTATION := "store_queue_max"
# ANNOTATION := "replay_queue_max"
# ANNOTATION := "mshr_busy"
# ANNOTATION := "schedule_stage_stall"
# ANNOTATION := "exception"

FUZZ_INPUT_FILEPATH := "${WORKDIR}/code.s"
FUZZ_GEN_COMMAND := ""
FUZZ_BUILD_COMMAND := "make \
  -C /rsd/Processor/Src/Verification/TestCode/Asm \
  SRC_DIRS=${WORKDIR}"
FUZZ_SIM_COMMAND := "make \
  -C /rsd/Processor/Src \
  -f Makefile.verilator.mk run \
  TEST_CODE=${WORKDIR}"

define FUZZ_ASM_HEADER
.option nopic
.text
.align    2
.globl    main
.type     main, @function
main:
    li      a0, 0xFFFFFFFF
    j       test_start
    .rept 0x40
    nop
    .endr
end:
    li      a7, 1
end2:
    j       end2
test_start:

endef

define FUZZ_ASM_FOOTER
    ret
test_memory:
endef
