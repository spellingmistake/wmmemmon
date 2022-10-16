/* $Id: mem_linux.c,v 1.5 2003/09/15 06:41:58 sch Exp $ */

/*
 * mem_linux.c - module to get memory/swap usages, for GNU/Linux
 *
 * Copyright(C) 2001,2002  Seiichi SATO <ssato@sh.rim.or.jp>
 * Copyright(C) 2001       John McCutchan <ttb@tentacle.dhs.org>
 *
 * licensed under the GPL
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/utsname.h>
#include "mem.h"

static int isnewformat = 0; /* for kernel 2.5.1 or later */

#ifdef DEBUG
#  define INLINE_STATIC static
#else
#  define INLINE_STATIC inline static
#endif

/* initialize function */
void
mem_init(void)
{
    struct utsname un;
    int version = 0, patchlevel = 0, sublevel = 0;
    char *p;

    /* get kernel version */
    if (uname(&un) == -1) {
	perror ("uname()");
    }
    p = strtok(un.release, ".");
    sscanf(p, "%d", &version);
    p = strtok(NULL, ".");
    sscanf(p, "%d", &patchlevel);
    p = strtok(NULL, ".");
    sscanf(p, "%d", &sublevel);

#ifdef DEBUG
    printf("version: %d\n", version);
    printf("patchlevel: %d\n", patchlevel);
    printf("sublevel: %d\n", sublevel);
#endif

    /* new format ? (kernel >= 2.5.1pre?) */
    /* see linux/fs/proc/proc_misc.c */
    if(((version * 10000) + (patchlevel * 100) + sublevel) >= 20501)
	isnewformat = 1;
}


INLINE_STATIC char *
skip_line (const char *p)
{
    while (*p != '\n') p++;
    return (char *) ++p;
}

INLINE_STATIC char *
skip_token (const char *p)
{
    while (isspace(*p)) p++;
    while (*p && !isspace(*p)) p++;
    return (char *)p;
}

INLINE_STATIC char *
skip_multiple_token (const char *p, int count)
{
    int i;
    for (i = 0; i < count; i++) p = skip_token (p);
    return (char *)p;
}

/* return mem/swap usage in percent 0 to 100 */
void
mem_getusage(int *per_mem, int *per_swap, const struct mem_options *opts)
{
    char buffer[BUFSIZ], *p;
    int fd, len, i;
    u_int64_t mtotal, mused, mfree, mshared, mbuffer, mcached;
    u_int64_t stotal, sused, sfree;

    /* read /proc/meminfo */
    fd = open("/proc/meminfo", O_RDONLY);
    if (fd < 0) {
	perror("can't open /proc/meminfo");
	exit(1);
    }
    len = read(fd, buffer, BUFSIZ - 1);
    if (len < 0) {
	perror("can't read /proc/meminfo");
	exit(1);
    }
    close(fd);

    buffer[len] = '\0';
    p = buffer;

    if (!isnewformat) {
	/* skip 3 lines */
	for (i = 0; i < 3; i++)
	  p = skip_line(p);
    }

    p = skip_token(p);
    /* examine each line of file */
    mtotal  = strtoul(p, &p, 0); p = skip_multiple_token(p, 2);
    mfree   = strtoul(p, &p, 0); p = skip_multiple_token(p, 2);
    mshared = strtoul(p, &p, 0); p = skip_multiple_token(p, 2);
    mbuffer = strtoul(p, &p, 0); p = skip_multiple_token(p, 2);
    mcached = strtoul(p, &p, 0);

    /* skip N lines and examine info about swap */
    /* kernel-2.4.2:N=8  2.4.16:N=7  */
    while (isprint(p[0])) {
	p = skip_line(p);
	if (strncmp(p, "SwapTotal", 9) == 0) break;
    }

    p = skip_token(p);
    stotal = strtoul(p, &p, 0); p = skip_multiple_token(p, 2);
    sfree  = strtoul(p, &p, 0);

    /* calculate memory usage in percent */
    mused = mtotal - mfree;
    if (opts->ignore_buffers)
	mused -= mbuffer;
    if (opts->ignore_cached)
	mused -= mcached;
    *per_mem = 100 * (double) mused / (double) mtotal;
    if (*per_mem > 97) *per_mem = 100;

    /* calculate swap usage in percent */
    sused = stotal - sfree;
    if (!stotal) {
	*per_swap = 0;
    } else {
	*per_swap = 100 * (double) sused / (double) stotal;
    }

#if DEBUG
    printf("-----------------------\n");
    printf("MemTotal:  %12ld\n", (unsigned long)mtotal);
    printf("MemFree:   %12ld\n", (unsigned long)mfree);
    printf("MemShared: %12ld\n", (unsigned long)mshared);
    printf("Buffers:   %12ld\n", (unsigned long)mbuffer);
    printf("Cached:    %12ld\n", (unsigned long)mcached);
    printf("SwapTotal: %12ld\n", (unsigned long)stotal);
    printf("SwapFree:  %12ld\n", (unsigned long)sfree);
    printf("-----------------------\n\n");
#endif

}
