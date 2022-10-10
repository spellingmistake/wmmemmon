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

/* return mem/swap usage in percent 0 to 100 */
void mem_getusage(int *memory_usage[2], const struct mem_options *opts __UNUSED)
{
	MEMORYSTATUS mstat;

	GlobalMemoryStatus(&mstat);

	*memory_usage[0] = 100 * (double)(mstat.dwTotalPhys - mstat.dwAvailPhys) /
		(double)mstat.dwTotalPhys;

	if (mstat.dwTotalVirtual)
	{
		*memory_usage[1] = 100 * (double)mstat.dwTotalVirtual -
			(mstat.dwAvailVirtual / (double)mstat.dwTotalVirtual);
	}
	else
	{
		*memory_usage[1] = 0;
	}


}
