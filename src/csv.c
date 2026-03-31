/* csv.c - CSV/delimited-text tokenizer
 *
 * Shared by the sc binary (for native CSV loading) and psc (for delimited
 * text conversion).  This file has no curses or sc-global dependencies so
 * it can be compiled into both binaries without #ifdef gymnastics.
 */

#include "csv.h"
#include <ctype.h>

void csv_reader_init(csv_reader *r, FILE *fp, int delim1, int delim2,
                     int strip_delim, int strnums, int plainnums)
{
    r->fp          = fp;
    r->delim1      = delim1;
    r->delim2      = delim2;
    r->strip_delim = strip_delim;
    r->strnums     = strnums;
    r->plainnums   = plainnums;
    r->token[0]    = '\0';
}

/*
 * Read the next token from the input stream.
 * Returns one of CSV_END, CSV_NUM, CSV_ALPHA, CSV_SEP, CSV_EOL.
 * On CSV_NUM or CSV_ALPHA the text is in r->token.
 */
int csv_next(csv_reader *r)
{
    int   c;
    char *p;
    int   founddigit;

    p = r->token;
    c = getc(r->fp);

    if (c == EOF)
        return CSV_END;

    if (c == '\n')
        return CSV_EOL;

    if (c == r->delim1 || (r->delim2 != 0 && c == r->delim2))
    {
        if (r->strip_delim)
        {
            while ((c = getc(r->fp)) != EOF &&
                   (c == r->delim1 || (r->delim2 != 0 && c == r->delim2)))
                ;
            (void)ungetc(c, r->fp);
        }
        return CSV_SEP;
    }

    /* quoted field */
    if (c == '"')
    {
        while ((c = getc(r->fp)) != EOF && c != '"' && c != '\n')
            *p++ = (char)c;
        if (c != '"')
            (void)ungetc(c, r->fp);
        *p = '\0';
        return CSV_ALPHA;
    }

    /* unquoted field: read until delimiter, newline, or EOF */
    while (c != r->delim1 && !(r->delim2 != 0 && c == r->delim2) &&
           c != '\n' && c != EOF)
    {
        *p++ = (char)c;
        c    = getc(r->fp);
    }
    *p = '\0';
    (void)ungetc(c, r->fp);

    /* decide if the token looks like a number */
    p          = r->token;
    c          = (unsigned char)*p;
    founddigit = 0;

    if (!r->strnums && (isdigit(c) || c == '.' || c == '-' || c == '+'))
    {
        int lastprtnum = 0;

        while (isdigit(c) || c == '.' ||
               (!r->plainnums &&
                (c == '-' || c == '+' || c == 'e' || c == 'E')))
        {
            if (isdigit(c))
                lastprtnum = founddigit = 1;
            else if (!(c == '.' || c == 'e' || c == 'E'))
                lastprtnum = 0;
            c = (unsigned char)*p++;
        }
        if (c == '\0' && founddigit && lastprtnum)
            return CSV_NUM;
    }

    return CSV_ALPHA;
}
