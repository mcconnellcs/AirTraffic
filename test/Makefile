# Host unit tests for the parts of AirTraffic that don't need the board:
# math, parsing, formatting, animation and gestures.
#
#   make            build + run the tests
#   make coverage   also print a line-coverage report (clang/llvm)
#
# ArduinoJson is header-only; point ARDUINOJSON at its "src" folder.

CXX         ?= c++
SKETCH      := ../firmware/AirTraffic
ARDUINOJSON ?= $(HOME)/Documents/Arduino/libraries/ArduinoJson/src
BUILD       := build

SOURCES := $(SKETCH)/geo.cpp $(SKETCH)/format.cpp $(SKETCH)/parsers.cpp \
           $(SKETCH)/gesture.cpp $(SKETCH)/sky_model.cpp $(SKETCH)/settings_rules.cpp \
           $(SKETCH)/demo_flights.cpp
TESTS   := $(wildcard test_*.cpp)

CXXFLAGS := -std=c++17 -O0 -g -Wall -Wextra -Werror -Wno-missing-field-initializers \
            -I$(SKETCH) -Ivendor -I$(ARDUINOJSON) -DFIXTURE_DIR=\"$(CURDIR)/fixtures\"

.PHONY: test coverage clean

test: $(BUILD)/tests
	./$(BUILD)/tests

$(BUILD)/tests: $(SOURCES) $(TESTS) main.cpp $(wildcard $(SKETCH)/*.h)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(COVFLAGS) main.cpp $(TESTS) $(SOURCES) -o $@

coverage: clean
	$(MAKE) COVFLAGS="-fprofile-instr-generate -fcoverage-mapping" $(BUILD)/tests
	LLVM_PROFILE_FILE=$(BUILD)/tests.profraw ./$(BUILD)/tests
	$(LLVM_PROFDATA) merge -sparse $(BUILD)/tests.profraw -o $(BUILD)/tests.profdata
	$(LLVM_COV) report $(BUILD)/tests -instr-profile=$(BUILD)/tests.profdata $(SOURCES) $(SKETCH)/anim.h

LLVM_PROFDATA ?= $(shell xcrun --find llvm-profdata 2>/dev/null || echo llvm-profdata)
LLVM_COV      ?= $(shell xcrun --find llvm-cov 2>/dev/null || echo llvm-cov)

clean:
	rm -rf $(BUILD)
