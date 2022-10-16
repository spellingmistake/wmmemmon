/* $Id: mem_cygwin.c,v 1.1.1.1 2002/10/14 12:23:26 sch Exp $ */

/*
 * mem_cygwin.c - module to get memory/swap usages, for Cygwin
 *
 * Copyright(C) 2002  Seiichi SATO <ssato@sh.rim.or.jp>
 *
 * licensed under the GPL
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <w32api/windows.h>
#include <w32api/winbase.h>
#include "mem.h"

/* initialize function */
void
mem_init(void)
{
    return;
}

/* return mem/swap usage in percent 0 to 100 */
void
mem_getusage(int *per_mem, int *per_swap, const struct mem_options *dummy)
{
    MEMORYSTATUS mstat;

    GlobalMemoryStatus(&mstat);

    *per_mem = 100 * (double)(mstat.dwTotalPhys - mstat.dwAvailPhys) /
		(double)mstat.dwTotalPhys;

    if (mstat.dwTotalVirtual == 0) {
	*per_swap = 0;
	return;
    }

    *per_swap = 100 * (double)mstat.dwTotalVirtual - mstat.dwAvailVirtual /
		(double)mstat.dwTotalVirtual;

}
