/* test_sc_csv.c - Integration tests for CSV loading in sc.
 *
 * These tests write a small CSV to a temp file, run the sc binary with the
 * 'P' (print) command, and verify the cells were loaded correctly.
 */

#define _POSIX_C_SOURCE 200809L  /* expose popen, pclose, strdup on Linux */

#include "unity/unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SC_BIN
#define SC_BIN "../bin/mac/sc/sc"
#endif

void setUp(void)    {}
void tearDown(void) {}

/* Write content to a temp file and return its path. Caller must remove it. */
static void write_temp_csv(const char *path, const char *content)
{
    FILE *f = fopen(path, "w");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "failed to create temp CSV");
    fputs(content, f);
    fclose(f);
}

/* Run 'echo P | sc <path>' and return the output as a malloc'd string. */
static char *sc_print(const char *path)
{
    char   cmd[512];
    char   buf[65536];
    size_t total = 0;
    FILE  *fp;

    snprintf(cmd, sizeof(cmd), "echo 'P' | %s %s 2>/dev/null", SC_BIN, path);
    fp = popen(cmd, "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(fp, "popen failed");

    while (!feof(fp) && total < sizeof(buf) - 1)
        total += fread(buf + total, 1, sizeof(buf) - 1 - total, fp);
    pclose(fp);

    buf[total] = '\0';
    return strdup(buf);
}

/* ------------------------------------------------------------------ */

void test_header_row_loads_as_strings(void)
{
    const char *path = "/tmp/sc_test_header.csv";
    write_temp_csv(path, "name,age,city\n");

    char *out = sc_print(path);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "leftstring A0 = \"name\""), "A0 should be 'name'");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "leftstring B0 = \"age\""),  "B0 should be 'age'");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "leftstring C0 = \"city\""), "C0 should be 'city'");

    free(out);
    remove(path);
}

void test_numeric_cells_load_as_numbers(void)
{
    const char *path = "/tmp/sc_test_nums.csv";
    write_temp_csv(path, "name,score\nAlice,42\n");

    char *out = sc_print(path);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "leftstring A1 = \"Alice\""), "A1 should be 'Alice'");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "let B1 = 42"),              "B1 should be 42");

    free(out);
    remove(path);
}

void test_float_loads_as_number(void)
{
    const char *path = "/tmp/sc_test_float.csv";
    write_temp_csv(path, "val\n3.14\n");

    char *out = sc_print(path);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "let A1 = 3.14"), "A1 should be 3.14");

    free(out);
    remove(path);
}

void test_quoted_field_with_comma_loads_as_string(void)
{
    const char *path = "/tmp/sc_test_quoted.csv";
    write_temp_csv(path, "\"London, UK\",100\n");

    char *out = sc_print(path);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "London, UK"), "A0 should contain 'London, UK'");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "let B0 = 100"), "B0 should be 100");

    free(out);
    remove(path);
}

void test_empty_field_skips_cell(void)
{
    /* a,,b  =>  A0="a", B0 empty, C0="b" */
    const char *path = "/tmp/sc_test_empty.csv";
    write_temp_csv(path, "a,,b\n");

    char *out = sc_print(path);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "leftstring A0 = \"a\""), "A0 should be 'a'");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "leftstring C0 = \"b\""), "C0 should be 'b'");

    free(out);
    remove(path);
}

void test_multiple_rows_correct_row_index(void)
{
    const char *path = "/tmp/sc_test_rows.csv";
    write_temp_csv(path, "r0\nr1\nr2\n");

    char *out = sc_print(path);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "leftstring A0 = \"r0\""), "row 0");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "leftstring A1 = \"r1\""), "row 1");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "leftstring A2 = \"r2\""), "row 2");

    free(out);
    remove(path);
}

void test_multiline_quoted_field_does_not_shift_rows(void)
{
    const char *path = "/tmp/sc_test_multiline.csv";
    write_temp_csv(path, "name,addr,n\r\nBob,\"1 Main St\r\nApt 2\",5\r\nAmy,x,6\r\n");

    char *out = sc_print(path);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "leftstring B1 = \"1 Main St Apt 2\""), "B1 joined");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "let C1 = 5"),                "C1 should be 5");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "leftstring A2 = \"Amy\""),   "A2 should be 'Amy'");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "let C2 = 6"),                "C2 should be 6");

    free(out);
    remove(path);
}

void test_save_to_sc_file_preserves_csv(void)
{
    /* Load a CSV and explicitly save to .sc — the original .csv must be unchanged */
    const char *csv_path = "/tmp/sc_test_save.csv";
    const char *sc_path  = "/tmp/sc_test_save.sc";
    char        cmd[512];
    int         ret;

    write_temp_csv(csv_path, "x,y\n1,2\n");
    remove(sc_path);

    snprintf(cmd, sizeof(cmd),
             "printf 'put \"%s\"\\n' | %s %s 2>/dev/null",
             sc_path, SC_BIN, csv_path);
    ret = system(cmd);
    (void)ret;

    /* .sc output file should have been created */
    FILE *f = fopen(sc_path, "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "explicit put should create .sc file");
    if (f) fclose(f);

    /* original CSV must still be unchanged */
    f = fopen(csv_path, "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "original .csv should still exist");
    if (f)
    {
        char buf[64];
        (void)fgets(buf, sizeof(buf), f);
        fclose(f);
        TEST_ASSERT_EQUAL_STRING("x,y\n", buf);
    }

    remove(csv_path);
    remove(sc_path);
}

void test_sc_output_contains_expected_cells(void)
{
    /* Verify the .sc file written from a CSV has correct cell assignments */
    const char *csv_path = "/tmp/sc_test_cells.csv";
    const char *sc_path  = "/tmp/sc_test_cells.sc";
    char        cmd[512];
    char        buf[4096];
    int         ret;

    write_temp_csv(csv_path, "name,score\nAlice,99\n");
    remove(sc_path);

    snprintf(cmd, sizeof(cmd),
             "printf 'put \"%s\"\\n' | %s %s 2>/dev/null",
             sc_path, SC_BIN, csv_path);
    ret = system(cmd);
    (void)ret;

    FILE *f = fopen(sc_path, "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "sc file should exist after put");

    buf[0] = '\0';
    if (f)
    {
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        buf[n] = '\0';
        fclose(f);
    }

    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "leftstring A0 = \"name\""),  "A0=name");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "leftstring B0 = \"score\""), "B0=score");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "leftstring A1 = \"Alice\""), "A1=Alice");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "let B1 = 99"),               "B1=99");

    remove(csv_path);
    remove(sc_path);
}

/* ------------------------------------------------------------------ */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_header_row_loads_as_strings);
    RUN_TEST(test_numeric_cells_load_as_numbers);
    RUN_TEST(test_float_loads_as_number);
    RUN_TEST(test_quoted_field_with_comma_loads_as_string);
    RUN_TEST(test_empty_field_skips_cell);
    RUN_TEST(test_multiple_rows_correct_row_index);
    RUN_TEST(test_multiline_quoted_field_does_not_shift_rows);
    RUN_TEST(test_save_to_sc_file_preserves_csv);
    RUN_TEST(test_sc_output_contains_expected_cells);

    return UNITY_END();
}
