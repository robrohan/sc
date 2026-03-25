CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall -Wextra -I. -I../src
UNITY   = unity/unity.c

UNAME := $(shell uname -s)
ifeq ($(UNAME), Darwin)
    PLATFORM := mac
else
    PLATFORM := linux
endif
SC_BIN := ../bin/$(PLATFORM)/sc/sc

# Each test binary is built from its test file + unity + the module under test
TEST_BINS = test_csv test_sc_csv

.PHONY: all clean

all: $(TEST_BINS)
	@echo "--- Running tests ---"
	@for t in $(TEST_BINS); do ./$$t; done

test_csv: test_csv.c unity/unity.c ../src/csv.c
	$(CC) $(CFLAGS) -o $@ $^

test_sc_csv: test_sc_csv.c unity/unity.c
	$(CC) $(CFLAGS) -DSC_BIN=\"$(SC_BIN)\" -o $@ $^

clean:
	rm -f $(TEST_BINS)
