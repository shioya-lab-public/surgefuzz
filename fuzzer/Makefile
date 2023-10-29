TARGET := fuzz_main

SRC_DIR := src
INC_DIR := include

CXX ?= g++
CXXFLAGS := -std=c++2a -Wall -Wextra -g -fconcepts
CXXFLAGS += -O2
CXXFLAGS += -I$(INC_DIR)
CXXFLAGS += -I../harness

CXXFLAGS += -DTARGET_COVERAGE_BITS=8

ifeq ($(POWER_SCHEDULE), enable)
	CXXFLAGS += -DENABLE_POWER_SCHEDULE
endif

ifeq ($(FUZZER), directfuzz)
	CXXFLAGS += -DENABLE_POWER_SCHEDULE
endif

ifeq ($(DEBUG_PRINT), enable)
	CXXFLAGS += -DENABLE_DEBUG_PRINT
endif

SRCS := \
	$(SRC_DIR)/common.cpp \
	$(SRC_DIR)/corpus.cpp \
	$(SRC_DIR)/executor.cpp \
	$(SRC_DIR)/feedback.cpp \
	$(SRC_DIR)/fuzzer.cpp \
	$(SRC_DIR)/program.cpp \
	$(SRC_DIR)/state.cpp \
	$(SRC_DIR)/util.cpp \
	$(SRC_DIR)/fuzz_main.cpp

OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

format:
	clang-format -i $(SRC_DIR)/*.cpp
	find $(INC_DIR) -name "*.hpp" | xargs clang-format -i

test_build:
	make clean && make -j
	make clean && POWER_SCHEDULE=enable make -j
	make clean && POWER_SCHEDULE=enable make -j
	make clean && POWER_SCHEDULE=enable DEBUG_PRINT=enable make -j

clean:
	rm -rf $(OBJS) $(TARGET)

.PHONY: all test format test_build clean
