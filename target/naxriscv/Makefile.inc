ANNOTATION := lq_usage
# ANNOTATION := sq_usage
# ANNOTATION := branch_miss
# ANNOTATION := dcache_conflict

FUZZ_INPUT_FILEPATH := ${WORKDIR}/NaxSoftware/baremetal/fuzz/src/crt.S
FUZZ_GEN_COMMAND := ""
FUZZ_BUILD_COMMAND := "make -C /workdir/NaxSoftware/baremetal/fuzz \
  RISCV_PATH=/riscv \
  MARCH=rv32im MABI=ilp32"
FUZZ_SIM_COMMAND := "/NaxRiscv/src/test/cpp/naxriscv/obj_dir/VNaxRiscv \
  --load-elf ${WORKDIR}/NaxSoftware/baremetal/fuzz/build/fuzz.elf \
  --seed=0 \
  --timeout=20000 \
  --pass-symbol pass \
  --stats-print-all \
  --output-dir=${WORKDIR}"

define FUZZ_ASM_HEADER
  #include "../../driver/riscv_asm.h"

  #define FLAG_V (1 << 0)
  #define FLAG_R (1 << 1)
  #define FLAG_W (1 << 2)
  #define FLAG_X (1 << 3)
  #define FLAG_U (1 << 4)
  #define FLAG_G (1 << 5)
  #define FLAG_A (1 << 6)
  #define FLAG_D (1 << 7)

  #define mmu_set_tree(table, virtual, next) \
      la t0, table; li t2, ((virtual >> 20) & 0xFFC); add t0, t0, t2; \
      la t1, next; srli t1, t1, 2; ori t1, t1, 0x01; \
      sw t1, 0(t0)

  #define mmu_set_leaf(table, virtual, physical, flags) \
      la t0, table; li t2, ((virtual >> 10) & 0xFFC); add t0, t0, t2; \
      li t1, physical; srli t1, t1, 2; ori t1, t1, flags; \
      sw t1, 0(t0)

  #define mmu_enable(root) \
      la t0, root; srli t0, t0, 12; \
      li t1, 0x1 << 31; or t0, t0, t1; \
      sfence.vma; \
      csrw satp, t0; \
      fence.i

  #define machine_to_user(func) \
      la t0, func; csrw mepc, t0; \
      li t0, MSTATUS_MPP_USER; csrw mstatus, t0; \
      mret

  .globl _start
  _start:
      la x1, trap
      csrw mtvec, x1

      # Map program into user space
      mmu_set_leaf(MMU_TABLE_1, 0x80000000, 0x80000000,
          (FLAG_D | FLAG_A | FLAG_U | FLAG_X | FLAG_V))
      # Map data into user space
      mmu_set_leaf(MMU_TABLE_1, 0x80001000, 0x80001000,
          (FLAG_D | FLAG_A | FLAG_U | FLAG_R | FLAG_V))
      mmu_set_leaf(MMU_TABLE_1, 0x80002000, 0x80002000,
          (FLAG_D | FLAG_A | FLAG_U | FLAG_W | FLAG_R | FLAG_V))

      mmu_set_tree(MMU_TABLE_0, 0x80000000, MMU_TABLE_1)

      # Map IO into user space
      mmu_set_leaf(MMU_TABLE_2, 0x10000000, 0x10000000,
          (FLAG_D | FLAG_A | FLAG_U | FLAG_W | FLAG_R | FLAG_V));
      mmu_set_tree(MMU_TABLE_0, 0x10000000, MMU_TABLE_2);

      mmu_enable(MMU_TABLE_0)

      machine_to_user(user_code)

      .rept 0x100
      nop
      .endr
  user_code:

endef

define FUZZ_ASM_FOOTER
  delay:
      .rept 0x100
      nop
      .endr

  trap:
      addi t0, t0, 1
      addi t1, t0, 1
      addi t2, t1, 1
      addi t3, t2, 1
      li x31, 4
  redo_A:
      la x1, test_memory
      li x10, 0x99887700
      li x11, 0x99887701
      li x14, 0x99887704
      li x15, 0x99887705
      li x16, 0x99887706
      sw x10, 0(x1)
      sw x11, 4(x1)
      lw x5, 0(x1)
      lw x5, 4(x1)
      sw x10, 0(x1)
      lw x5, 4(x1)
      addi x1, x1, 64
      addi x2, x2, 64
      addi x31, x31, -1
      bnez x31, redo_A

  pass:
      j pass

  .align 12
  test_memory:
      .rept 0x100
      .word 0x27e91b60
      .word 0xa656bbbb
      .word 0x59014f57
      .word 0xe339e4d3
      .endr
      .rept 0x100
      .word 0x1546ce54
      .word 0xaa010ccf
      .word 0x6bbe2419
      .word 0xa3cc949e
      .endr

  .align 12
  MMU_TABLE_0:
  .word 0

  .align 12
  MMU_TABLE_1:
  .word 0

  .align 12
  MMU_TABLE_2:
  .word 0
endef
