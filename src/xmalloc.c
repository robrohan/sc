/*
 * A safer saner malloc, for careless programmers
 * $Revision: 7.16 $
 */

#include <curses.h>
#include "sc.h"

#include <stdlib.h>

void fatal(char *str);

#define	MAGIC	(double)1234567890.12344

char *
scxmalloc(unsigned n)
{
	register char *ptr;

	if ((ptr = (char *)malloc(n + sizeof(double))) == NULL)
		fatal("scxmalloc: no memory");
	*((double *)(void *) ptr) = MAGIC;		/* magic number */
	return(ptr + sizeof(double));
}

/* we make sure realloc will do a malloc if needed */
char *
scxrealloc(char *ptr, unsigned n)
{
	if (ptr == NULL)
		return(scxmalloc(n));

	ptr -= sizeof(double);
	if (*((double *)(void *) ptr) != MAGIC)
		fatal("scxrealloc: storage not scxmalloc'ed");

	if ((ptr = (char *)realloc(ptr, n + sizeof(double))) == NULL)
		fatal("scxmalloc: no memory");
	*((double *)(void *) ptr) = MAGIC;		/* magic number */
	return(ptr + sizeof(double));
}

void
scxfree(char *p)
{
	if (p == NULL)
		fatal("scxfree: NULL");
	p -= sizeof(double);
	if (*((double *)(void *) p) != MAGIC)
		fatal("scxfree: storage not malloc'ed");
	free(p);
}

#ifdef PSC
void
fatal(char *str)
{
    (void) fprintf(stderr,"%s\n", str);
    exit(1);
}
#else
void
fatal(char *str)
{
    deraw(1);
    (void) fprintf(stderr,"%s\n", str);
    diesave();
    exit(1);
}
#endif /* PSC */
