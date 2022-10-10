/*
 * mem_freebsd.c - module to get memory/swap usages in percent, for FreeBSD
 *
 * Copyright (c) 2001, 2002 Seiichi SATO <ssato@sh.rim.or.jp>
 *
 * licensed under the GPL
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#include <sys/vmmeter.h>

#include "mem.h"

static inline void debug_memory_output(struct vmtotal *vm)
{
#ifdef DEBUG
	fprintf(stdout, "--------------------------------------------\n");
	fprintf(stdout, "t_vm      total virtual memory         %6lu\n", vm->t_vm);
	fprintf(stdout, "t_rm      total real memory in use     %6lu\n", vm->t_rm);
	fprintf(stdout, "t_arm     active real memory           %6lu\n", vm->t_arm);
	fprintf(stdout, "t_rmshr   shared real memory           %6lu\n", vm->t_rmshr);
	fprintf(stdout, "t_armshr  active shared real memory    %6lu\n", vm->t_armshr);
	fprintf(stdout, "t_free    free memory pages            %6lu\n", vm->t_free);
	fprintf(stdout, "--------------------------------------------\n");
#else		/* #ifdef DEBUG */
		(void)vm;
#endif		/* #ifdef DEBUG */
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
	/* FIXME: the total usage(t_rm and t_free) is incorrect?? */
	if (0 < vm.t_rm)
	{
		*memory_usage[0] = 100 * (double)vm.t_rm / (double)(vm.t_rm + vm.t_free);
	}
	if (95 < *memory_usage[0])
	{
		*memory_usage[0] = 100;
	}

	/* get swap usage */
	/* not written yet.. can i get swap usage via sysctl? */
	*memory_usage[1] = 0;

	debug_memory_output(&vm);
}
