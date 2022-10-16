/* $Id: mem_irix.c,v 1.1.1.1 2002/10/14 12:23:26 sch Exp $ */

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
#include "mem.h"

#include <sys/sysinfo.h>
#include <sys/sysget.h>
#include <sys/swap.h>

/* initialize function */
void mem_init(void)
{
  /* Nothing to initialize on IRIX */
  return;
}

/* return memory/swap usage in percent 0 to 100 */
void mem_getusage(int *per_mem, int *per_swap, const struct mem_options *opts)
{
    struct sgt_cookie cookie;
    nodeinfo_t        info;
    long long         mem_total  = 0;
    long long         mem_free   = 0;
    off_t             swap_total = 0;
    off_t             swap_free  = 0;

    /* get mem usage */
    SGT_COOKIE_INIT(&cookie);
    /* fprintf(stderr, "!!!DEBUG\n"); */
    while(cookie.sc_status != SC_DONE) {
      sysget(SGT_NODE_INFO, (char *)&info, sizeof(info), SGT_READ, &cookie);
      mem_total += info.totalmem;
      mem_free  += info.freemem;
      /* fprintf(stderr, "!!!node: %d/%d (mem free)\n", info.freemem,
         info.totalmem); */
    };

    /* get swap usage */
    if (swapctl(SC_GETSWAPMAX, &swap_total) == -1) {
      fprintf(stderr, "wmmemmon: Error getting swap size info.\n");
      swap_total = 1;
    };
    if (swapctl(SC_GETFREESWAP, &swap_free) == -1) {
      fprintf(stderr, "wmmemmon: Error getting free swap info.\n");
      swap_free = 0;
    };
    /*    fprintf(stderr, "!!!total: %lu/%lu (swap free)\n",
            (unsigned long)swap_free,
            (unsigned long)swap_total);*/

    /* calc mem/swap usage in percent */
    *per_mem = 100 * (1 - ((double)mem_free / (double)mem_total));
    *per_swap = 100 * (1 - ((double)swap_free / (double)swap_total));
    /*fprintf(stderr, "%d%% mem used, %d%% swap used\n", *per_mem,
     *per_swap);*/
}
