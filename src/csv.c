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
static int is_delim(const csv_reader *r, int c)
{
    return c == r->delim1 || (r->delim2 != 0 && c == r->delim2);
}

/* Append c to the token, silently truncating overlong fields */
static void put_tok(csv_reader *r, char **p, int c)
{
    if (*p < r->token + CSV_TOKEN_SIZE - 1)
        *(*p)++ = (char)c;
}

int csv_next(csv_reader *r)
{
    int   c;
    char *p;
    int   founddigit;

    p = r->token;
    c = getc(r->fp);

    if (c == EOF)
        return CSV_END;

    /* accept \n, \r\n and bare \r line endings */
    if (c == '\r')
    {
        if ((c = getc(r->fp)) != '\n')
            (void)ungetc(c, r->fp);
        return CSV_EOL;
    }
    if (c == '\n')
        return CSV_EOL;

    if (is_delim(r, c))
    {
        if (r->strip_delim)
        {
            while ((c = getc(r->fp)) != EOF && is_delim(r, c))
                ;
            (void)ungetc(c, r->fp);
        }
        return CSV_SEP;
    }

    /* quoted field (RFC 4180): "" is a literal quote and delimiters or
     * line breaks inside the quotes belong to the field.  Line breaks
     * become spaces since a cell label can't hold them.  Any text after
     * the closing quote is kept up to the next delimiter. */
    if (c == '"')
    {
        for (;;)
        {
            c = getc(r->fp);
            if (c == EOF)
                break;
            if (c == '"')
            {
                if ((c = getc(r->fp)) != '"')
                    break;
            }
            else if (c == '\r' || c == '\n')
            {
                if (c == '\r' && (c = getc(r->fp)) != '\n')
                    (void)ungetc(c, r->fp);
                c = ' ';
            }
            put_tok(r, &p, c);
        }
        while (c != EOF && c != '\n' && c != '\r' && !is_delim(r, c))
        {
            put_tok(r, &p, c);
            c = getc(r->fp);
        }
        *p = '\0';
        if (c != EOF)
            (void)ungetc(c, r->fp);
        return CSV_ALPHA;
    }

    /* unquoted field: read until delimiter, line break, or EOF */
    while (!is_delim(r, c) && c != '\n' && c != '\r' && c != EOF)
    {
        put_tok(r, &p, c);
        c = getc(r->fp);
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
