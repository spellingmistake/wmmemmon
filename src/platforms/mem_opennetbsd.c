/*
 * mem_opennetbsd.c - module to get memory/swap usages in percent, for
 * OpenBSD and NetBSD
 *
 * Copyright (c) 2001,2002 Seiichi SATO <ssato@sh.rim.or.jp>
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

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/swap.h>

#include "mem.h"

static inline void debug_memory_output(struct vmtotal *vm)
{
#ifdef DEBUG
	fprintf(stdout, "--------------------------------------------\n");
	fprintf(stdout, "t_rm      total real memory in use     %6d\n", vm.t_rm);
	fprintf(stdout, "t_arm     active real memory           %6d\n", vm.t_arm);
	fprintf(stdout, "t_rmshr   shared real memory           %6d\n", vm.t_rmshr);
	fprintf(stdout, "t_armshr  active shared real memory    %6d\n", vm.t_armshr);
	fprintf(stdout, "t_free    free memory pages            %6d\n", vm.t_free);
	fprintf(stdout, "--------------------------------------------\n");
#else		/* #ifdef DEBUG */
	(void)vm;
#endif		/* #ifdef DEBUG */
}

/* get swap usage */
static int get_swap_usage(void)
{
	struct swapent *swap_dev;
	int num_swap, stotal, sused, i;

	stotal = sused = 0;

	if ((num_swap = swapctl(SWAP_NSWAP, 0, 0)))
	{
		if (!(swap_dev = malloc(num_swap * sizeof(*swap_dev))))
		{
			if (-1 != swapctl(SWAP_STATS, swap_dev, num_swap))
			{
				for (i = 0; i < num_swap; ++i)
				{
					if (swap_dev[i].se_flags & SWF_ENABLE)
					{
						stotal += swap_dev[i].se_nblks;
						sused += swap_dev[i].se_inuse;
					}
				}
			}

			free(swap_dev);

			if (!sused)
			{
				return 0;
			}
		}
	}

	return sused ? (100 * (double)sused / (double)stotal) : 0;
}

/* return mem/swap usage in percent 0 to 100 */
void mem_getusage(int *memory_usage[2], const struct mem_options *opts __UNUSED)
{
	struct vmtotal vm;
	size_t size = sizeof(vm);
	static int mib[] = { CTL_VM, VM_METER };

	/* get mem usage */
	if (0 > sysctl(mib, 2, &vm, &size, NULL, 0))
	{
		bzero(&vm, sizeof(vm));
	}

	/* calc mem usage in percent */
	if (0 < vm.t_rm)
	{
		*memory_usage[0] = 100 * (double)vm.t_rm / (double)(vm.t_rm + vm.t_free);
	}

	if (97 < *memory_usage[0])
	{
		*memory_usage[0] = 100;
	}

	/* swap */
	*memory_usage[1] = get_swap_usage();

	debug_memory_output(&vm);
}
