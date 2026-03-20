/* test_csv.c - Unit tests for csv.c tokenizer */

#include "unity/unity.h"
#include "../src/csv.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* helpers                                                             */
/* ------------------------------------------------------------------ */

/*
 * Write s into a tmpfile and rewind it.  More portable than fmemopen:
 * avoids the const-buffer and size=0 issues on macOS.
 */
static FILE *
open_buf(const char *s)
{
    FILE *fp = tmpfile();
    fputs(s, fp);
    rewind(fp);
    return fp;
}

/* Initialise a csv_reader with comma delimiter, no stripping, plain mode */
static void
init_csv(csv_reader *r, FILE *fp)
{
    csv_reader_init(r, fp, ',', 0, 0, 0, 0);
}

void setUp(void)    {}
void tearDown(void) {}

/* ------------------------------------------------------------------ */
/* basic single-token tests                                            */
/* ------------------------------------------------------------------ */

void test_integer_is_num(void)
{
    FILE *fp = open_buf("42\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_NUM, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("42", r.token);
    fclose(fp);
}

void test_float_is_num(void)
{
    FILE *fp = open_buf("3.14\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_NUM, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("3.14", r.token);
    fclose(fp);
}

void test_negative_is_num(void)
{
    FILE *fp = open_buf("-7\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_NUM, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("-7", r.token);
    fclose(fp);
}

void test_string_is_alpha(void)
{
    FILE *fp = open_buf("hello\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("hello", r.token);
    fclose(fp);
}

void test_eol(void)
{
    FILE *fp = open_buf("\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_EOL, csv_next(&r));
    fclose(fp);
}

void test_eof_on_empty(void)
{
    FILE *fp = open_buf("");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_END, csv_next(&r));
    fclose(fp);
}

/* ------------------------------------------------------------------ */
/* separator tests                                                     */
/* ------------------------------------------------------------------ */

void test_separator_returned(void)
{
    FILE *fp = open_buf(",\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_SEP, csv_next(&r));
    fclose(fp);
}

void test_two_fields_on_one_line(void)
{
    FILE *fp = open_buf("foo,bar\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("foo", r.token);

    TEST_ASSERT_EQUAL_INT(CSV_SEP, csv_next(&r));

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("bar", r.token);

    TEST_ASSERT_EQUAL_INT(CSV_EOL, csv_next(&r));
    fclose(fp);
}

void test_num_and_string_fields(void)
{
    FILE *fp = open_buf("Name,42\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("Name", r.token);

    TEST_ASSERT_EQUAL_INT(CSV_SEP, csv_next(&r));

    TEST_ASSERT_EQUAL_INT(CSV_NUM, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("42", r.token);
    fclose(fp);
}

/* ------------------------------------------------------------------ */
/* empty field (consecutive commas)                                    */
/* ------------------------------------------------------------------ */

void test_empty_middle_field(void)
{
    /* "a,,b" -> ALPHA sep SEP sep ALPHA eol */
    FILE *fp = open_buf("a,,b\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("a", r.token);

    TEST_ASSERT_EQUAL_INT(CSV_SEP, csv_next(&r));

    /* empty field between the two commas */
    TEST_ASSERT_EQUAL_INT(CSV_SEP, csv_next(&r));

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("b", r.token);
    fclose(fp);
}

/* ------------------------------------------------------------------ */
/* quoted field tests                                                  */
/* ------------------------------------------------------------------ */

void test_quoted_string(void)
{
    FILE *fp = open_buf("\"hello world\"\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("hello world", r.token);
    fclose(fp);
}

void test_quoted_field_with_comma(void)
{
    FILE *fp = open_buf("\"a,b\",c\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("a,b", r.token);

    TEST_ASSERT_EQUAL_INT(CSV_SEP, csv_next(&r));

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("c", r.token);
    fclose(fp);
}

void test_quoted_number_is_alpha(void)
{
    /* A quoted number should come back as ALPHA, not NUM */
    FILE *fp = open_buf("\"42\"\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("42", r.token);
    fclose(fp);
}

/* ------------------------------------------------------------------ */
/* strnums flag                                                        */
/* ------------------------------------------------------------------ */

void test_strnums_treats_number_as_alpha(void)
{
    FILE *fp = open_buf("99\n");
    csv_reader r;
    csv_reader_init(&r, fp, ',', 0, 0, 1 /* strnums */, 0);

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("99", r.token);
    fclose(fp);
}

/* ------------------------------------------------------------------ */
/* multi-line input                                                    */
/* ------------------------------------------------------------------ */

void test_two_rows(void)
{
    FILE *fp = open_buf("1,2\n3,4\n");
    csv_reader r;
    init_csv(&r, fp);

    TEST_ASSERT_EQUAL_INT(CSV_NUM, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("1", r.token);
    TEST_ASSERT_EQUAL_INT(CSV_SEP,  csv_next(&r));
    TEST_ASSERT_EQUAL_INT(CSV_NUM, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("2", r.token);
    TEST_ASSERT_EQUAL_INT(CSV_EOL,  csv_next(&r));

    TEST_ASSERT_EQUAL_INT(CSV_NUM, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("3", r.token);
    TEST_ASSERT_EQUAL_INT(CSV_SEP,  csv_next(&r));
    TEST_ASSERT_EQUAL_INT(CSV_NUM, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("4", r.token);
    TEST_ASSERT_EQUAL_INT(CSV_EOL,  csv_next(&r));

    TEST_ASSERT_EQUAL_INT(CSV_END,  csv_next(&r));
    fclose(fp);
}

/* ------------------------------------------------------------------ */
/* alternate delimiter                                                 */
/* ------------------------------------------------------------------ */

void test_tab_delimiter(void)
{
    FILE *fp = open_buf("foo\tbar\n");
    csv_reader r;
    csv_reader_init(&r, fp, '\t', 0, 0, 0, 0);

    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("foo", r.token);
    TEST_ASSERT_EQUAL_INT(CSV_SEP,   csv_next(&r));
    TEST_ASSERT_EQUAL_INT(CSV_ALPHA, csv_next(&r));
    TEST_ASSERT_EQUAL_STRING("bar", r.token);
    fclose(fp);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_integer_is_num);
    RUN_TEST(test_float_is_num);
    RUN_TEST(test_negative_is_num);
    RUN_TEST(test_string_is_alpha);
    RUN_TEST(test_eol);
    RUN_TEST(test_eof_on_empty);
    RUN_TEST(test_separator_returned);
    RUN_TEST(test_two_fields_on_one_line);
    RUN_TEST(test_num_and_string_fields);
    RUN_TEST(test_empty_middle_field);
    RUN_TEST(test_quoted_string);
    RUN_TEST(test_quoted_field_with_comma);
    RUN_TEST(test_quoted_number_is_alpha);
    RUN_TEST(test_strnums_treats_number_as_alpha);
    RUN_TEST(test_two_rows);
    RUN_TEST(test_tab_delimiter);

    return UNITY_END();
}
