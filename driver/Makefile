INC_DIR := include
TEST_DIR := test

CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++14 -g -O2
CXXFLAGS += -fconcepts
CXXFLAGS += -I$(INC_DIR)

TEST_SRC := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJ := $(TEST_SRC:.cpp=.o)
TEST_EXEC := test_main

FORMAT_INC := $(shell find $(INC_DIR) -name *.hpp)
FORMAT_TEST := $(wildcard $(TEST_DIR)/*.cpp $(TEST_DIR)/*.hpp)

$(TEST_EXEC): $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_OBJ)

test: $(TEST_EXEC)
	./$(TEST_EXEC)

format:
	clang-format -i $(FORMAT_INC) $(FORMAT_TEST)

clean:
	rm -f $(TEST_OBJ) $(TEST_EXEC)

.PHONY: test format clean
