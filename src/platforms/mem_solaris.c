/*
 * mem_solaric.c - module to get memory/swap usages in percent, for Solaris
 *
 * Copyright (c) 2001 Jonathan Lang <lang@synopsys.com>
 * Copyright (c) 2002 Seiichi SATO <ssato@sh.rim.or.jp>
 *
 * licensed under the GPL
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/swap.h>

#include "mem.h"

/* return memory/swap usage in percent 0 to 100 */
void mem_getusage(int *memory_usage[2], const struct mem_options *opts __UNUSED)
{
	struct anoninfo anon;
	static int mem_total;
	static int mem_free;
	static int swap_total;
	static int swap_free;

	/* get mem usage */
	mem_total = sysconf(_SC_PHYS_PAGES);
	mem_free = sysconf(_SC_AVPHYS_PAGES);

	/* get swap usage */
	if (-1 != swapctl(SC_AINFO, &anon))
	{
		swap_total = anon.ani_max;
		swap_free = anon.ani_max - anon.ani_resv;

		/* calc mem/swap usage in percent */
		*memory_usage[0] = 100 * (1 - ((double)mem_free / (double)mem_total));
		*memory_usage[1] = 100 * (1 - ((double)swap_free / (double)swap_total));
	}
	else
	{
		exit(1);
	}
}
