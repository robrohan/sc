CC      = clang
CFLAGS  = -std=c99 -O2 -Wall -Wextra -I. -I../src
UNITY   = unity/unity.c

UNAME := $(shell uname -s)
ifeq ($(UNAME), Darwin)
    PLATFORM := mac
else
    PLATFORM := linux
endif
SC_BIN := ../bin/$(PLATFORM)/sc/sc

# Binaries go in a per-platform dir so mac and linux builds don't collide
BUILD := build/$(PLATFORM)

# Each test binary is built from its test file + unity + the module under test
TEST_BINS = $(BUILD)/test_csv $(BUILD)/test_sc_csv

.PHONY: all clean

all: $(TEST_BINS)
	@echo "--- Running tests ---"
	@for t in $(TEST_BINS); do ./$$t || exit 1; done

$(BUILD):
	mkdir -p $@

$(BUILD)/test_csv: test_csv.c unity/unity.c ../src/csv.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/test_sc_csv: test_sc_csv.c unity/unity.c | $(BUILD)
	$(CC) $(CFLAGS) -DSC_BIN=\"$(SC_BIN)\" -o $@ $^

clean:
	rm -rf build test_csv test_sc_csv
