# ---------------------------------------------------
# Configurable variables
# ---------------------------------------------------

# Core target selection. Uncomment the desired one.
TARGET_CORE := rsd
# TARGET_CORE := boom
# TARGET_CORE := naxriscv

# Fuzzer selection. Uncomment the desired one.
FUZZER := surgefuzz
# FUZZER := rfuzz
# FUZZER := difuzzrtl
# FUZZER := directfuzz
# FUZZER := blackbox

# SELECTION_METHOD is used to decide the method of selecting the ancestor register for SurgeFuzz.
# - closer: selects based on proximity.
# - closer_mi: utilizes proximity selection with added pruning based on mutual information.
SELECTION_METHOD := closer
# SELECTION_METHOD := closer_mi

# POWER_SCHEDULE is a configuration for SurgeFuzz.
# When enabled, it allows fuzzing with power schedule.
POWER_SCHEDULE := enable

# COV_BIT defines the size of the coverage bitmap as 1 << COV_BIT for DifuzzRTL and SurgeFuzz.
COV_BIT := 20

# SEARCH_BIT determines the depth of the search for ancestor registers in SurgeFuzz.
# It's crucial to ensure an adequate size for this parameter.
SEARCH_BIT := 500

# MAX_ENERGY and MIN_ENERGY define constant values used for the power schedule in DirectFuzz.
MAX_ENERGY := 10
MIN_ENERGY := 0

# FUZZ_RND_SEED defines the random seed for fuzzing.
FUZZ_RND_SEED := 0

# FUZZ_MAX_CYCLE determines the number of fuzzing cycles to be executed.
# TIMEOUT_SEC specifies the duration to execute in seconds.
# Either value must be set to zero. If both are set,
# fuzzing will terminate when either condition is met.
TIMEOUT_SEC := 300 # 5 minutes = 300 seconds
FUZZ_MAX_CYCLE := 0

# Paths for docker
SHARED := /shared
OUT := /out
WORKDIR := /workdir

# Include target specific makefile configurations
include target/$(TARGET_CORE)/Makefile.inc


# ---------------------------------------------------
# Build and Run Targets
# ---------------------------------------------------

# Image name for the docker build
IMAGE_NAME := surgefuzz-$(TARGET_CORE)-$(FUZZER)-$(ANNOTATION)

all: run

# Build the docker image
build:
	docker build -t $(IMAGE_NAME) \
		--build-arg USER_UID=$(shell id -u) \
		--build-arg USER_GID=$(shell id -g) \
		--build-arg TARGET_CORE=$(TARGET_CORE) \
		--build-arg FUZZER=$(FUZZER) \
		--build-arg ANNOTATION=$(ANNOTATION) \
		--build-arg SELECTION_METHOD=$(SELECTION_METHOD) \
		--build-arg POWER_SCHEDULE=$(POWER_SCHEDULE) \
		--build-arg COV_BIT=$(COV_BIT) \
		--build-arg SEARCH_BIT=$(SEARCH_BIT) \
		--build-arg MAX_ENERGY=$(MAX_ENERGY) \
		--build-arg MIN_ENERGY=$(MIN_ENERGY) \
		--build-arg SHARED=$(SHARED) \
		--build-arg OUT=$(OUT) \
		--build-arg WORKDIR=$(WORKDIR) \
		-f target/$(TARGET_CORE)/Dockerfile \
		.

# Run the built docker image
export FUZZ_ASM_HEADER
export FUZZ_ASM_FOOTER
run: build
	docker run -it \
		--env=FUZZ_RND_SEED=$(FUZZ_RND_SEED) \
		--env=FUZZ_MAX_CYCLE=$(FUZZ_MAX_CYCLE) \
		--env=TIMEOUT_SEC=$(TIMEOUT_SEC) \
		--env=FUZZ_MODE=$(FUZZER) \
		--env=FUZZ_GEN_COMMAND=$(FUZZ_GEN_COMMAND) \
		--env=FUZZ_BUILD_COMMAND=$(FUZZ_BUILD_COMMAND) \
		--env=FUZZ_SIM_COMMAND=$(FUZZ_SIM_COMMAND) \
		--env=FUZZ_ASM_HEADER="$$FUZZ_ASM_HEADER" \
		--env=FUZZ_ASM_FOOTER="$$FUZZ_ASM_FOOTER" \
		--env=FUZZ_INPUT_FILEPATH=$(FUZZ_INPUT_FILEPATH) \
		$(IMAGE_NAME)
