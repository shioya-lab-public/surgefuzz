FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-c"]

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
        zip \
        zlib1g-dev

# Install Bazel
RUN apt install -y apt-transport-https curl gnupg && \
    curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor >bazel-archive-keyring.gpg && \
    mv bazel-archive-keyring.gpg /usr/share/keyrings && \
    echo "deb [arch=amd64 signed-by=/usr/share/keyrings/bazel-archive-keyring.gpg] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
RUN apt update && \
    apt install bazel

RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Create an user
ARG USERNAME=user
ARG USER_UID=1000
ARG USER_GID=$USER_UID
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME

# Install anaconda
# https://github.com/conda-forge/miniforge/#unix-like-platforms-mac-os--linux
ENV CONDA /conda
RUN curl -L -O "https://github.com/conda-forge/miniforge/releases/latest/download/Mambaforge-$(uname)-$(uname -m).sh" && \
    bash /Mambaforge-$(uname)-$(uname -m).sh -b -p ${CONDA} && \
    rm /Mambaforge-$(uname)-$(uname -m).sh
USER user:user
RUN echo 'source "${CONDA}/etc/profile.d/conda.sh"' >> ~/.bash_profile && \
    echo 'source ~/.bash_profile' >> ~/.bashrc
USER root:root

# Install Yosys
RUN git clone https://github.com/YosysHQ/yosys && \
    cd yosys && \
    git checkout yosys-0.27 && \
    make config-clang && \
    make -j $(nproc) && \
    make install

# Install Verible
RUN git clone https://github.com/chipsalliance/verible.git /verible && \
    cd /verible && \
    git checkout v0.0-3110-gbb28b0b4 && \
    bazel run -c opt :install -- /usr/local/bin

# Setup chipyard
# https://chipyard.readthedocs.io/en/stable/Chipyard-Basics/Initial-Repo-Setup.html#initial-repository-setup
ENV CHIPYARD /chipyard
RUN git clone https://github.com/ucb-bar/chipyard.git ${CHIPYARD} && \
    cd ${CHIPYARD} && \
    git checkout 1.8.1 && \
    source "${CONDA}/etc/profile.d/conda.sh" && \
    conda install -n base conda-lock && \
    conda activate base && \
    ./build-setup.sh riscv-tools && \
    chown -R user:user ${CHIPYARD}

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
RUN [ "/bin/bash", "-c", "source ${CONDA}/etc/profile.d/conda.sh; source ${CHIPYARD}/env.sh; ${TARGET_SCRIPT_ROOT}/build/main.sh" ]
ENTRYPOINT [ "/bin/bash", "-c", "source ${CONDA}/etc/profile.d/conda.sh; source ${CHIPYARD}/env.sh; ${TARGET_SCRIPT_ROOT}/run/main.sh" ]
