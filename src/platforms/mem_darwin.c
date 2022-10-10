/*
 * mem_darwin.c - module to get memory/swap usages in percent, for Darwin
 *
 * Copyright(c) 2002 Landon J. Fuller <landonf@opendarwin.org>
 *
 * licensed under the GPL
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <mach/mach.h>

#include "mem.h"

static mach_port_t host_port;
static host_size;
static vm_size_t pagesize;

/* initialize function */
void mem_init(void) __CTOR
{
    host_port = mach_host_self();
    host_size = sizeof(vm_statistics_data_t)/sizeof(integer_t);
    host_page_size(host_port, &pagesize);
}

/* return mem/swap usage in percent 0 to 100 */
void mem_getusage(int *memory_usage[2], const struct mem_options *opts __UNUSED)
{
	kern_return_t ret;
    vm_statistics_data_t vm_stat;
    unsigned long long mem_free, mem_total, mem_used;

	ret = host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat,
			&host_size) ;

	if (KERN_SUCCESS == ret)
	{
		mem_used = (vm_stat.active_count + vm_stat.inactive_count +
				vm_stat.wire_count) * pagesize;
		mem_free = vm_stat.free_count * pagesize;
		mem_total = mem_used + mem_free;

		*memory_usage[0] = (100 * ((double)mem_free / (double)mem_total));
		/* XXX Can I get VM statistics from mach (default_pager_info ?) */
		*memory_usage[1] = 0;
	}
	else
	{
        exit(1);
	}
}
