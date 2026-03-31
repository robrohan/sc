/* Sc parse routine
 *
 * usage psc options
 * options:
 *   -L		Left justify strings.  Default is right justify.
 *   -r		Assemble data into rows first, not columns.
 *   -R	n	Increment by n between rows
 *   -C n	Increment by n between columns
 *   -n n	Length of the row (column) should be n.
 *   -s v	Top left location in the spreadsheet should be v; eg, k5
 *   -d c       Use c as the delimiter between the fields.
 *   -k         Keep all delimiters - Default is strip multiple delimiters to 1.
 *   -f         suppress 'format' lines in output
 *   -S		Use strings vs numbers for numbers
 *   -P		Use numbers only when there is no [-+eE] (plain numbers only)
 *
 *  Author: Robert Bond
 *  Adjustments: Jeff Buhrt, Eric Putz and Chuck Martin
 */
char *rev = "$Revision: 7.16 $";

#include "csv.h"
#include "sc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *coltoa(int col);
char *progname;
int   getrow(char *p);
int   getcol(char *p);

int *fwidth;
int *precision;
int  maxcols;
int *realfmt;

int curlen;
int curcol, coff;
int currow, roff;
int first;
int effr, effc;

/* option flags */
int  colfirst  = FALSE;
int  leftadj   = FALSE;
int  r0        = 0;
int  c0        = 0;
int  rinc      = 1;
int  cinc      = 1;
int  len       = 20000;
int  drop_format = FALSE;

int main(int argc, char **argv)
{
    int        c;
    int        i, j;
    char      *p;
    csv_reader r;
    int        delim1      = ' ';
    int        delim2      = '\t';
    int        strip_delim = TRUE;
    int        strnums     = FALSE;
    int        plainnums   = FALSE;

    progname = argv[0];
    while ((c = getopt(argc, argv, "rfLks:R:C:n:d:SPv")) != EOF)
    {
        switch (c)
        {
        case 'r':
            colfirst = TRUE;
            break;
        case 'L':
            leftadj = TRUE;
            break;
        case 's':
            c0 = getcol(optarg);
            r0 = getrow(optarg);
            break;
        case 'R':
            rinc = atoi(optarg);
            break;
        case 'C':
            cinc = atoi(optarg);
            break;
        case 'n':
            len = atoi(optarg);
            break;
        case 'd':
            delim1 = (unsigned char)optarg[0];
            delim2 = 0;
            break;
        case 'k':
            strip_delim = FALSE;
            break;
        case 'f':
            drop_format = TRUE;
            break;
        case 'S':
            strnums = TRUE;
            break;
        case 'P':
            plainnums = TRUE;
            break;
        case 'v':
            (void)fprintf(stderr, "%s: %s\n", progname, rev);
            __attribute__((fallthrough));
        default:
            (void)fprintf(stderr,
                          "Usage: %s [-rkfLSPv] [-s v] [-R i] [-C i] [-n i] [-d c]\n",
                          progname);
            exit(1);
        }
    }

    if (optind < argc)
    {
        (void)fprintf(stderr, "Usage: %s [-rL] [-s v] [-R i] [-C i] [-n i] [-d c]\n",
                      progname);
        exit(1);
    }

    /* setup the spreadsheet arrays */
    if (!growtbl(GROWNEW, 0, 0))
        exit(1);

    csv_reader_init(&r, stdin, delim1, delim2, strip_delim, strnums, plainnums);

    curlen = 0;
    curcol = c0;
    coff   = 0;
    currow = r0;
    roff   = 0;
    first  = TRUE;

    while (1)
    {
        effr = currow + roff;
        effc = curcol + coff;

        switch (csv_next(&r))
        {
        case CSV_END:
            if (drop_format)
                exit(0);
            for (i = 0; i < maxcols; i++)
            {
                if (fwidth[i])
                    (void)printf("format %s %d %d %d\n", coltoa(i),
                                 fwidth[i] + 1, precision[i], REFMTFIX);
            }
            exit(0);
        case CSV_NUM:
            first = FALSE;
            (void)printf("let %s%d = %s\n", coltoa(effc), effr, r.token);
            if (effc >= maxcols - 1)
            {
                if (!growtbl(GROWCOL, 0, effc))
                {
                    (void)fprintf(stderr, "Invalid column used: %s\n",
                                  coltoa(effc));
                    continue;
                }
            }
            i = 0;
            j = 0;
            p = r.token;
            while (*p && *p != '.')
            {
                p++;
                i++;
            }
            if (*p)
            {
                p++;
                i++;
            }
            while (*p)
            {
                p++;
                i++;
                j++;
            }
            {
                int ow, nw;

                ow = fwidth[effc] - precision[effc];
                if (precision[effc] < j)
                    precision[effc] = j;

                if (fwidth[effc] < i)
                    fwidth[effc] = i;

                if ((nw = i - j) > ow)
                    fwidth[effc] += nw - (fwidth[effc] - precision[effc]);
            }
            break;
        case CSV_ALPHA:
            first = FALSE;
            if (leftadj)
                (void)printf("leftstring %s%d = \"%s\"\n", coltoa(effc), effr,
                             r.token);
            else
                (void)printf("rightstring %s%d = \"%s\"\n", coltoa(effc), effr,
                             r.token);
            if (effc >= maxcols - 1)
            {
                if (!growtbl(GROWCOL, 0, effc))
                {
                    (void)fprintf(stderr, "Invalid column used: %s\n",
                                  coltoa(effc));
                    continue;
                }
            }
            i = (int)strlen(r.token);
            if (i > fwidth[effc])
                fwidth[effc] = i;
            break;
        case CSV_SEP:
            if (first && strip_delim)
                break;
            if (colfirst)
                roff++;
            else
                coff++;
            break;
        case CSV_EOL:
            curlen++;
            roff  = 0;
            coff  = 0;
            first = TRUE;
            if (colfirst)
            {
                if (curlen >= len)
                {
                    curcol  = c0;
                    currow += rinc;
                    curlen  = 0;
                }
                else
                {
                    curcol += cinc;
                }
            }
            else
            {
                if (curlen >= len)
                {
                    currow  = r0;
                    curcol += cinc;
                    curlen  = 0;
                }
                else
                {
                    currow += rinc;
                }
            }
            break;
        }
    }
}

/* turns [A-Z][A-Z] into a column number */
int getcol(char *p)
{
    int col;

    col = 0;
    if (!p)
        return (0);
    while (*p && !isalpha((unsigned char)*p))
        p++;
    if (!*p)
        return (0);
    col = (toupper((unsigned char)*p) - 'A');
    if (isalpha((unsigned char)*++p))
        col = (col + 1) * 26 + (toupper((unsigned char)*p) - 'A');
    return (col);
}

/* given a string turn it into a row number */
int getrow(char *p)
{
    int row;

    row = 0;
    if (!p)
        return (0);
    while (*p && !isdigit((unsigned char)*p))
        p++;
    if (!*p)
        return (0);
    while (*p && isdigit((unsigned char)*p))
    {
        row = row * 10 + *p - '0';
        p++;
    }
    return (row);
}

/* turns a column number into [A-Z][A-Z] */
char *coltoa(int col)
{
    static char rname[3];
    char       *p = rname;

    if (col < 0 || col > 27 * 26)
        (void)fprintf(stderr, "coltoa: invalid col: %d", col);

    if (col > 25)
    {
        *p++ = (char)(col / 26 + 'A' - 1);
        col %= 26;
    }
    *p++ = (char)(col + 'A');
    *p   = '\0';
    return (rname);
}
