FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-c"]

RUN apt-get update && \
    apt-get -y install --no-install-suggests --no-install-recommends \
        autoconf \
        automake \
        autotools-dev \
        bc \
        bison \
        build-essential \
        ca-certificates \
        ccache \
        clang \
        clang-format \
        curl \
        device-tree-compiler \
        flex \
        gawk \
        git \
        gperf \
        graphviz \
        haskell-stack \
        libboost-all-dev \
        libexpat-dev \
        libffi-dev \
        libfl2 \
        libfl-dev \
        libgmp-dev \
        libmpc-dev \
        libmpfr-dev \
        libreadline-dev \
        libtool \
        nano \
        netbase \
        patchutils \
        python3 \
        python3-dev \
        python3-pip \
        python-is-python3 \
        tcl-dev \
        texinfo \
        unzip \
        xdot \
        wget \
        zip \
        zlib1g-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Create an user
ARG USERNAME=user
ARG USER_UID=1000
ARG USER_GID=${USER_UID}
RUN groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME}

# Install Verilator
RUN git clone https://github.com/verilator/verilator && \
    unset VERILATOR_ROOT && \
    cd verilator && \
    git checkout v4.110 && \
    autoconf && \
    export VERILATOR_ROOT=$(pwd) && \
    ./configure && \
    make -j $(nproc) && \
    make install && \
    rm -rf verilator

# Install Yosys
RUN git clone https://github.com/YosysHQ/yosys && \
    cd yosys && \
    git checkout 6e907ac && \
    make config-clang && \
    make -j $(nproc) && \
    make install && \
    rm -rf yosys

# Install sv2v
RUN git clone https://github.com/zachjs/sv2v.git && \
    cd sv2v && \
    git checkout effeded && \
    stack upgrade --binary-only && \
    make -j$(nproc)
ENV PATH $PATH:/sv2v/bin

# Install RISC-V toolchain
ENV RISCV /riscv
RUN mkdir $RISCV && \
    curl -SL https://github.com/lowRISC/lowrisc-toolchains/releases/download/20220210-1/lowrisc-toolchain-gcc-rv32imc-20220210-1.tar.xz | tar -xJC $RISCV
ENV PATH $PATH:$RISCV/lowrisc-toolchain-gcc-rv32imc-20220210-1/bin

# Clone rsd
ENV RSD_ROOT /rsd
RUN git clone https://github.com/rsd-devel/rsd && \
    cd ${RSD_ROOT} && \
    git checkout 5bc4a20
RUN chown -R user:user ${RSD_ROOT}

# Set env for rsd scripts
ENV RSD_GCC_PREFIX riscv32-unknown-elf-
ENV RSD_GCC_PATH /riscv/lowrisc-toolchain-gcc-rv32imc-20220210-1/bin

# Set env for fuzzer
ARG TARGET_CORE
ENV TARGET_CORE ${TARGET_CORE}
ARG FUZZER
ENV FUZZER ${FUZZER}
ARG SELECTION_METHOD
ENV SELECTION_METHOD ${SELECTION_METHOD}
ARG MAX_BIT
ENV MAX_BIT ${MAX_BIT}
ARG COV_BIT
ENV COV_BIT ${COV_BIT}
ARG SEARCH_BIT
ENV SEARCH_BIT ${SEARCH_BIT}
ARG MAX_ENERGY
ENV MAX_ENERGY ${MAX_ENERGY}
ARG MIN_ENERGY
ENV MIN_ENERGY ${MIN_ENERGY}
ARG ANNOTATION
ENV ANNOTATION ${ANNOTATION}
ARG POWER_SCHEDULE
ENV POWER_SCHEDULE ${POWER_SCHEDULE}

ENV FUZZER_ROOT /fuzzer
ENV SCRIPT_ROOT /script
ENV PASS_ROOT /pass
ENV DRIVER_ROOT /driver
ENV PATCH_ROOT /patch
ENV TARGET_SCRIPT_ROOT /target_script
COPY --chown=user:user fuzzer ${FUZZER_ROOT}
COPY --chown=user:user script ${SCRIPT_ROOT}
COPY --chown=user:user pass ${PASS_ROOT}
COPY --chown=user:user driver ${DRIVER_ROOT}
COPY --chown=user:user target/${TARGET_CORE}/patch ${PATCH_ROOT}
COPY --chown=user:user target/${TARGET_CORE}/script ${TARGET_SCRIPT_ROOT}

# Build a custom yosys pass
RUN make -C ${PASS_ROOT} METHOD=${FUZZER} -j$(nproc)

# Test driver for fuzzer
RUN make -C ${DRIVER_ROOT} test -j$(nproc)

# Build fuzzer
RUN make -C ${FUZZER_ROOT} -j$(nproc)

# Install python packages
RUN pip install --upgrade pip minepy && \
    pip install -r ${SCRIPT_ROOT}/prepare/requirements.txt

# Install python packages for SurgeFuzz
RUN if [ "$FUZZER" = "surgefuzz" ]; then \
    cd ${SCRIPT_ROOT}/profile && \
    pip install -r requirements.txt; \
fi

ARG SHARED
ENV SHARED ${SHARED}
ARG WORKDIR
ENV WORKDIR ${WORKDIR}
ARG OUT
ENV OUT ${OUT}

RUN mkdir -p ${SHARED} ${OUT} ${WORKDIR} && \
    chown user:user ${SHARED} ${OUT} ${WORKDIR} && \
    chmod 744 ${SHARED} ${OUT} ${WORKDIR}

USER user:user

RUN [ "/bin/bash", "-c", "${TARGET_SCRIPT_ROOT}/build/main.sh" ]
ENTRYPOINT [ "/bin/bash", "-c", "${TARGET_SCRIPT_ROOT}/run/main.sh" ]
