FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
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
        gdb \
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
        libsdl2-2.0-0 \
        libsdl2-dev \
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
        zlib1g-dev

RUN apt-get install -y --no-install-recommends \
        gnupg2 \
        openjdk-8-jdk \
        scala && \
    echo "deb https://repo.scala-sbt.org/scalasbt/debian all main" | tee /etc/apt/sources.list.d/sbt.list && \
    echo "deb https://repo.scala-sbt.org/scalasbt/debian /" | tee /etc/apt/sources.list.d/sbt_old.list && \
    curl -sL "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x2EE0EA64E40A89B84B2DF73499E82A75642AC823" | apt-key add && \
    apt-get update && \
    apt-get install -y --no-install-recommends sbt && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Create an user
ARG USERNAME=user
ARG USER_UID=1000
ARG USER_GID=$USER_UID
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME

# # Install skd
# USER user:user
# RUN curl -s "https://get.sdkman.io" | bash

# # Install java and sbt
# RUN /bin/bash -c "source $HOME/.sdkman/bin/sdkman-init.sh; sdk version; sdk install java $(sdk list java | grep -o "\b8\.[0-9]*\.[0-9]*\-tem" | head -1); sdk install sbt"
# USER root:root

# Install Verilator
RUN git clone https://github.com/verilator/verilator && \
    unset VERILATOR_ROOT && \
    cd verilator && \
    git checkout v4.226 && \
    autoconf && \
    export VERILATOR_ROOT=$(pwd) && \
    ./configure && \
    make -j $(nproc) && \
    make install

# Install Yosys
RUN git clone https://github.com/YosysHQ/yosys && \
    cd yosys && \
    git checkout yosys-0.27 && \
    make config-clang && \
    make -j $(nproc) && \
    make install

# Install RISC-V toolchain
ENV RISCV /riscv
RUN mkdir $RISCV && \
    curl -SL https://github.com/lowRISC/lowrisc-toolchains/releases/download/20220524-1/lowrisc-toolchain-gcc-rv64imac-20220524-1.tar.xz | tar -xJC $RISCV --strip-components 1
ENV PATH $PATH:$RISCV/bin

# Setup repos
ENV SPINALHDL /SpinalHDL
ENV NAXRISCV /NaxRiscv
RUN git clone https://github.com/SpinalHDL/SpinalHDL.git ${SPINALHDL} && \
    cd ${SPINALHDL} && \
    git checkout 6be9217 && \
    git submodule update --init --recursive && \
    chown -R user:user ${SPINALHDL}
RUN git clone https://github.com/SpinalHDL/NaxRiscv.git ${NAXRISCV} && \
    cd ${NAXRISCV} && \
    git checkout 51440b6 && \
    git submodule update --init --recursive && \
    cd ext/NaxSoftware && ./init.sh && \
    chown -R user:user ${NAXRISCV}

# Build riscv-isa-sim (spike), used as a golden model during the sim to check the dut behaviour (lock-step)
RUN cd ${NAXRISCV}/ext/riscv-isa-sim && \
    mkdir build && \
    cd build && \
    ../configure --prefix=$RISCV --enable-commitlog  --without-boost --without-boost-asio --without-boost-regex && \
    make -j$(nproc) && \
    g++ --shared -L. -Wl,--export-dynamic -L/usr/lib/x86_64-linux-gnu  -Wl,-rpath,/lib  -o package.so spike.o  libspike_main.a  libriscv.a  libdisasm.a  libsoftfloat.a  libfesvr.a  libfdt.a -lpthread -ldl -lboost_regex -lboost_system -lpthread  -lboost_system -lboost_regex

# Install ELFIO, used to load elf file in the sim
RUN git clone https://github.com/serge1/ELFIO.git && \
    cd ELFIO && \
    git checkout d251da0 && \
    cp -r elfio /usr/include

ENV RISCV_TESTS /riscv_tests
RUN git clone https://github.com/riscv/riscv-tests ${RISCV_TESTS} && \
    cd ${RISCV_TESTS} && \
    git checkout 394937e && \
    git submodule update --init --recursive && \
    autoconf && \
    ./configure --prefix=${RISCV}/target && \
    make && \
    make install

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
RUN pip install --upgrade pip && \
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
