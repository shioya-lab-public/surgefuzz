ANNOTATION := annotation
METHOD ?= surgefuzz

ANNOTATION_DIR := annotation
METHOD_DIR := method
INCLUDE_DIR := include

all: $(ANNOTATION).so $(METHOD).so

$(ANNOTATION).so: $(ANNOTATION_DIR)/$(ANNOTATION).cc
	yosys-config --exec --cxx --cxxflags --ldflags \
		-o $@ -shared $^ --ldlibs \
		-I$(INCLUDE_DIR)

$(METHOD).so: $(METHOD_DIR)/$(METHOD).cc
	yosys-config --exec --cxx --cxxflags $(CXXFLAGS) --ldflags \
		-o $@ -shared $^ --ldlibs \
		-I$(INCLUDE_DIR)

format:
	clang-format -i $(ANNOTATION_DIR)/*.cc $(METHOD_DIR)/*.cc $(INCLUDE_DIR)/*.h

test_build:
	make METHOD=surgefuzz
	make METHOD=rfuzz
	make METHOD=difuzzrtl
	make METHOD=directfuzz
	make METHOD=blackbox_fuzzer
	make clean

clean:
	rm -f $(ANNOTATION).so $(METHOD).so $(ANNOTATION).d $(METHOD).d

.PHONY: all format test_build clean
