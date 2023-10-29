# SurgeFuzz

SurgeFuzz is a directed fuzzing method for RISC-V CPU designs.
It can efficiently verify specific corner cases with user annotations.

## Prerequisites

* Docker is installed.
* Docker daemon is running.

## Getting Started

Before running the fuzzing process, you first need to clone the repository and move to the directory:

```sh
git clone https://github.com/shioya-lab-public/surgefuzz.git
cd surgefuzz
```

## How to Run

The fuzzing process runs inside a Docker container.
To build a Docker image and run the fuzzing, execute the following command:

```sh
make TARGET_CORE=rsd ANNOTATION=load_queue_max
```

By setting the variables `TARGET_CORE` and `ANNOTATION` described below, you can run fuzzing for a specific configuration. For example, the command `make TARGET_CORE=rsd ANNOTATION=load_queue_max` sets the target CPU to `rsd` and annotation to `load_queue_max` to run fuzzing.

### TARGET_CORE

`TARGET_CORE` specifies the target CPU for fuzzing.
The currently available CPUs are as follows:

* `rsd`: [RSD: RISC-V Out-of-Order Superscalar Processor](https://github.com/rsd-devel/rsd)
* `boom`: [SonicBOOM: The Berkeley Out-of-Order Machine](https://github.com/riscv-boom/riscv-boom)
* `naxriscv`: [NaxRiscv](https://github.com/SpinalHDL/NaxRiscv)

### ANNOTATION

`ANNOTATION` specifies an annotation for the target CPU of fuzzing.
For each CPU, the currently available annotations are as follows:

* `rsd`
    * `exception`: Frequent exceptions
    * `schedule_state_stall`: Stall the scheduler longer
    * `mshr_busy`: Keep MSHR busy longer
    * `load_queue_max`: High load queue usage
    * `store_queue_max`: High store queue usage
    * `replay_queue_max`: High replay queue usage
* `boom`
    * `ldq_full`: Frequently fill the load queue
    * `store_blocked`: Block store commits longer
* `naxriscv`
    * `branch_miss`:  Frequent branch prediction misses
    * `dcache_conflict`: Frequent data cache way conflicts
    * `lq_usage`: High load queue usage
    * `sq_usage`: High store queue usage

### Additional Settings

If you want to change the execution time for fuzzing, specify `TIMEOUT_SEC`.
For example, to set a timeout of 3600 seconds, use:

```sh
make TARGET_CORE=rsd ANNOTATION=load_queue_max TIMEOUT_SEC=3600
```

## How to Check Results

The fuzzing results are displayed in the following format in the standard output within the container.
For each fuzzing cycle, the achieved surge score and the highest surge score achieved up to that cycle are displayed:

```text
[fuzz_cycle:     1],  [current surge score:   0],  [max surge score:   0]
[fuzz_cycle:     2],  [current surge score:   0],  [max surge score:   0]
[fuzz_cycle:     3],  [current surge score:   0],  [max surge score:   0]
[fuzz_cycle:     4],  [current surge score:   0],  [max surge score:   0]
[fuzz_cycle:     5],  [current surge score:   2],  [max surge score:   2]
[fuzz_cycle:     6],  [current surge score:   2],  [max surge score:   2]
[fuzz_cycle:     7],  [current surge score:   2],  [max surge score:   2]
[fuzz_cycle:     8],  [current surge score:   2],  [max surge score:   2]
[fuzz_cycle:     9],  [current surge score:   2],  [max surge score:   2]
                                 ...
[fuzz_cycle:   318],  [current surge score:   6],  [max surge score:  10]
[fuzz_cycle:   319],  [current surge score:   4],  [max surge score:  10]
[fuzz_cycle:   320],  [current surge score:   5],  [max surge score:  10]
[fuzz_cycle:   321],  [current surge score:   2],  [max surge score:  10]
[fuzz_cycle:   322],  [current surge score:  10],  [max surge score:  10]
[fuzz_cycle:   323],  [current surge score:   6],  [max surge score:  10]
[fuzz_cycle:   324],  [current surge score:  10],  [max surge score:  10]
[fuzz_cycle:   325],  [current surge score:   7],  [max surge score:  10]
[fuzz_cycle:   326],  [current surge score:  10],  [max surge score:  10]
[fuzz_cycle:   327],  [current surge score:   7],  [max surge score:  10]
```

Detailed results are stored in the /workdir directory within the Docker container.

## License

SurgeFuzz is released under the [Apache License, Version 2.0](https://opensource.org/licenses/Apache-2.0).

## Contact
If you have any problems or questions, please contact us at sugiyama [at] rsg.ci.i.u-tokyo.ac.jp.
