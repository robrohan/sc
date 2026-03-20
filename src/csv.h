#ifndef CSV_H
#define CSV_H

#include <stdio.h>

/* Token types returned by csv_next() */
#define CSV_END   0 /* end of input */
#define CSV_NUM   1 /* numeric field */
#define CSV_ALPHA 2 /* string field */
#define CSV_SEP   3 /* field separator */
#define CSV_EOL   4 /* end of line */

#define CSV_TOKEN_SIZE 1024

typedef struct
{
    FILE *fp;
    int   delim1;     /* primary delimiter */
    int   delim2;     /* secondary delimiter, 0 = none */
    int   strip_delim; /* collapse consecutive delimiters */
    int   strnums;    /* treat numeric-looking fields as strings */
    int   plainnums;  /* only plain digits count as numbers (no e/E/+/-) */
    char  token[CSV_TOKEN_SIZE];
} csv_reader;

void csv_reader_init(csv_reader *r, FILE *fp, int delim1, int delim2,
                     int strip_delim, int strnums, int plainnums);
int csv_next(csv_reader *r);

#endif /* CSV_H */
