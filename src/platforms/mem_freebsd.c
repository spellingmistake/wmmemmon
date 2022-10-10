/*
 * mem_freebsd.c - module to get memory/swap usages in percent, for FreeBSD
 *
 * Copyright(c) 2001 Seiichi SATO <ssato@sh.rim.or.jp>
 *
 * licensed under the GPL
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <kvm.h>
#include <sys/vmmeter.h>

#include "mem.h"

static int pageshift;
static kvm_t *kvm_data;
static struct nlist nlst[] = {
	{"_cp_time"},
	{"_cnt"}, {0}
};

/* initialize function */
void mem_init(void) __CTOR
{
	int pagesize;
	bool ok = false;

	pagesize = getpagesize();
	pageshift = 0;

	while (1 < pagesize)
	{
		pageshift++;
		pagesize >>= 1;
	}

	kvm_data = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open");

	if (kvm_data)
	{
		kvm_nlist(kvm_data, nlst);

		if (!nlst[0].n_type || !nlst[1].n_type)
		{
			/* drop setgid & setuid (the latter should not be there really) */
			seteuid(getuid());
			setegid(getgid());

			if (geteuid() == getuid() && getegid() == getgid())
			{
				ok = true;
			}
			else
			{
				fprintf(stderr, "unable to drop privileges");
			}
		}
		else
		{
			fprintf(stderr, "error extracting symbols");
		}

	}
	else
	{
		fprintf(stderr, "can't open kernel virtual memory");
	}

	if (!ok)
	{
		exit(1);
	}
}

static inline void debug_memory_output(struct vmmeter *vm)
{
#ifdef DEBUG
	fprintf(stdout, "-------------------\n");
	fprintf(stdout, "total:%10d\n", vm->v_page_count * vm->v_page_size);
	fprintf(stdout, "free :%10d\n", vm->v_free_count * vm->v_page_size);
	fprintf(stdout, "act  :%10d\n", vm->v_active_count * vm->v_page_size);
	fprintf(stdout, "inact:%10d\n", vm->v_inactive_count * vm->v_page_size);
	fprintf(stdout, "wired:%10d\n", vm->v_wire_count * vm->v_page_size);
	fprintf(stdout, "cache:%10d\n", vm->v_cache_count * vm->v_page_size);
	fprintf(stdout, "-------------------\n");
#else		/* #ifdef DEBUG */
	(void)vm;
#endif		/* #ifdef DEBUG */
}

/* return mem/swap usage in percent 0 to 100 */
void mem_getusage(int *memory_usage[2], const struct mem_options *opts)
{
	u_int mused;
	int bufspace, n;
	time_t cur_time;
	struct vmmeter vm;
	struct kvm_swap swap;
	/* statics */
	static int swap_firsttime = 1;
	static int swappgsin = -1;
	static int swappgsout = -1;
	static int swapmax = 0, swapused = 0;
	static time_t last_time_swap = 0;

	/* get mem usage */
	if (kvm_read(kvm_data, nlst[0].n_value, &bufspace, sizeof(bufspace)) ==
			sizeof(bufspace) &&
			kvm_read(kvm_data, nlst[1].n_value, &vm, sizeof(vm)) == sizeof(vm))
	{

		/* get swap usage
		 * only calculate when first time or when changes took place
		 * do not call it more than 1 time per 2 seconds
		 * otherwise it can eat up to 50% of CPU time on heavy swap activity */
		cur_time = time(NULL);
		if (swap_firsttime ||
				(((vm.v_swappgsin > swappgsin) || (vm.v_swappgsout > swappgsout))
				 && cur_time > last_time_swap + 1))
		{

			swapmax = 0;
			swapused = 0;

			n = kvm_getswapinfo(kvm_data, &swap, 1, 0);
			if (0 <= n && swap.ksw_total)
			{
				swapmax = swap.ksw_total;
				swapused = swap.ksw_used;
			}

			swap_firsttime = 0;
			last_time_swap = cur_time;
		}
		swappgsin = vm.v_swappgsin;
		swappgsout = vm.v_swappgsout;

		/* calc mem/swap usage in percent */
		mused = vm.v_page_count - vm.v_free_count;
		if (opts->ignore_wired)
		{
			mused -= vm.v_wire_count;
		}
		if (opts->ignore_cached)
		{
			mused -= vm.v_cache_count;
		}

		*memory_usage[0] = 100 * (double)mused / (double)vm.v_page_count;
		*memory_usage[1] = 100 * (double)swapused / (double)swapmax;

		if (*memory_usage[0] > 97)
		{
			*memory_usage[0] = 100;
		}
	}
	else
	{
		fprintf(stderr, "unable to obtain kernel virtual memory "
				"information: %s\n", kvm_geterr(kvm_data));
		exit(1);
	}

	debug_memory_output(&vm);
}
