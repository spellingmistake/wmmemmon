/*
 * mem_linux.c - module to get memory/swap usages, for GNU/Linux
 *
 * Copyright(C) 2001,2002  Seiichi SATO <ssato@sh.rim.or.jp>
 * Copyright(C) 2001       John McCutchan <ttb@tentacle.dhs.org>
 * Copyright (c) 2022       Thomas Egerer <thomas.egerer@port22.eu>
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
#include <fcntl.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include "mem.h"

enum memory_type_t {
	MTOTAL,
	MUSED,
	MFREE,
	MSHARED,
	MBUFFER,
	MCACHED,
	STOTAL,
	SUSED,
	SFREE,
};

static inline char *skip_line(const char *p)
{
	while (*p != '\n')
	{
		p++;
	}
	return (char *)++p;
}

static inline char *skip_token(const char *p)
{
	while (isspace(*p))
	{
		p++;
	}
	while (*p && !isspace(*p))
	{
		p++;
	}
	return (char *)p;
}

static inline char *skip_multiple_token(const char *p, int count)
{
	int i;

	for (i = 0; i < count; ++i)
	{
		p = skip_token(p);
	}
	return (char *)p;
}

static inline void debug_memory_output(u_int64_t mem[9])
{
#if DEBUG
	fprintf(stdout, "-----------------------\n");
	fprintf(stdout, "MemTotal:  %12" PRIu64 "ld\n", mem[MTOTAL]);
	fprintf(stdout, "MemFree:   %12" PRIu64 "ld\n", mem[MFREE]);
	fprintf(stdout, "MemShared: %12" PRIu64 "ld\n", mem[MSHARED]);
	fprintf(stdout, "Buffers:   %12" PRIu64 "ld\n", mem[MBUFFER]);
	fprintf(stdout, "Cached:    %12" PRIu64 "ld\n", mem[MCACHED]);
	fprintf(stdout, "SwapTotal: %12" PRIu64 "ld\n", mem[STOTAL]);
	fprintf(stdout, "SwapFree:  %12" PRIu64 "ld\n", mem[SFREE]);
	fprintf(stdout, "-----------------------\n\n");
#else		/* #ifdef DEBUG */
		(void)mem;
#endif		/* #ifdef DEBUG */
}

/* return mem/swap usage in percent 0 to 100 */
void mem_getusage(int *memory_usage, const mem_options_t *mem_opts)
{
	char buffer[BUFSIZ], *p;
	int fd, len, i;
	u_int64_t mem[9] = { 0 };

	/* read /proc/meminfo */
	fd = open("/proc/meminfo", O_RDONLY);

	if (0 > fd)
	{
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

	p = skip_token(p);
	/* examine each line of file */
	mem[MTOTAL] = strtoul(p, &p, 0); p = skip_multiple_token(p, 2);
	mem[MFREE] = strtoul(p, &p, 0); p = skip_multiple_token(p, 2);
	mem[MSHARED] = strtoul(p, &p, 0); p = skip_multiple_token(p, 2);
	mem[MBUFFER] = strtoul(p, &p, 0); p = skip_multiple_token(p, 2);
	mem[MCACHED] = strtoul(p, &p, 0);

	/* skip N lines and examine info about swap */
	/* kernel-2.4.2:N=8  2.4.16:N=7  */
	while (isprint(p[0]))
	{
		p = skip_line(p);
		if (strncmp(p, "SwapTotal", 9) == 0)
		{
			break;
		}
	}

	p = skip_token(p);
	mem[STOTAL] = strtoul(p, &p, 0); p = skip_multiple_token(p, 2);
	mem[SFREE]  = strtoul(p, &p, 0);

	/* calculate memory usage in percent */
	mem[MUSED] = mem[MTOTAL] - mem[MFREE];
	if (mem_opts->ignore_buffers)
	{
		mem[MUSED] -= mem[MBUFFER];
	}
	if (mem_opts->ignore_cached)
	{
		mem[MUSED] -= mem[MCACHED];
	}
	memory_usage[0] = 100 * (double)mem[MUSED] / (double)mem[MTOTAL];

	if (memory_usage[0] > 97)
	{
		memory_usage[0] = 100;
	}

	/* calculate swap usage in percent */
	mem[SUSED] = mem[STOTAL] - mem[SFREE];
	if (!mem[STOTAL])
	{
		memory_usage[1] = 0;
	}
	else
	{
		memory_usage[1] = 100 * (double)mem[SUSED] / (double)mem[STOTAL];
	}
}
