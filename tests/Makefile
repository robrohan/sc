CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall -Wextra -I. -I../src
UNITY   = unity/unity.c

# Each test binary is built from its test file + unity + the module under test
TEST_BINS = test_csv

.PHONY: all clean

all: $(TEST_BINS)
	@echo "--- Running tests ---"
	@for t in $(TEST_BINS); do ./$$t; done

test_csv: test_csv.c unity/unity.c ../src/csv.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TEST_BINS)
