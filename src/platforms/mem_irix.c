/*
 * mem_irix.c - module to get memory/swap usages in percent, for IRIX 6.5
 *
 * Copyright (c) 2002 Jonathan C. Patschke <jp@celestrion.net>
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

#include <sys/sysinfo.h>
#include <sys/sysget.h>
#include <sys/swap.h>

#include "mem.h"

/* return memory/swap usage in percent 0 to 100 */
void mem_getusage(int *memory_usage[2], const struct mem_options *opts __UNUSED)
{
	nodeinfo_t info;
	struct sgt_cookie cookie;
	off_t swap_total = 0, swap_free = 0;
	long long mem_total = 0, mem_free = 0;

	/* get mem usage */
	SGT_COOKIE_INIT(&cookie);

	while(cookie.sc_status != SC_DONE)
	{
		sysget(SGT_NODE_INFO, (char *)&info, sizeof(info), SGT_READ, &cookie);
		mem_total += info.totalmem;
		mem_free += info.freemem;
	};

	/* get swap usage */
	if (-1 == swapctl(SC_GETSWAPMAX, &swap_total))
	{
		fprintf(stderr, "wmmemmon: Error getting swap size info.\n");
		swap_total = 1;
	};

	if (-1 == swapctl(SC_GETFREESWAP, &swap_free))
	{
		fprintf(stderr, "wmmemmon: Error getting free swap info.\n");
		swap_free = 0;
	};

	/* calc mem/swap usage in percent */
	*memory_usage[0] = 100 * (1 - ((double)mem_free / (double)mem_total));
	*memory_usage[0] = 100 * (1 - ((double)swap_free / (double)swap_total));
}
