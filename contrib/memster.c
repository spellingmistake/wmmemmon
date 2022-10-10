#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define MAX_FACTOR 32

static int term;
static const char *prefixes[] = {
	"", "kibi", "mebi", "gibi", "tebi", "pebi", "exbi", NULL,
};

static void sighandler(int signal __attribute__((unused)))
{
	term = 1;
}

 __attribute__((noreturn)) static void usage(const char *prog)
{
	fprintf(stdout, "Usage: %s [FACTOR]\n\n"
			"Allocate memory in chunks of FACTOR * pagesize * 1024 every "
			"second.\nFACTOR must be in the range of [1; %u]\n",
			prog, MAX_FACTOR);
	exit(0);
}

static int memory_hamster(size_t offset)
{
	int ret = 0;
	const char *pfx;
	char *ptr = NULL;
	size_t total = 0, i, divisor = 1, x = 0;

	pfx = prefixes[x];
	do
	{
		total += offset;
		ptr = realloc(ptr, total);

		if (ptr)
		{
			for (i = offset; i < total; i += offset / 128)
			{	/* 'use' the memory to own it, but don't use memset(3) */
				ptr[i] = i;
			}

			if (1024 * 16 < (total / divisor) && (prefixes[x + 1]))
			{
				pfx = prefixes[++x];
				divisor *= 1024;
			}
			fprintf(stdout, "%zu %sbytes allocated.     \r",
					total / divisor, pfx);
			fflush(stdout);
			sleep(1);
		}
		else
		{
			ret = -1;
			fprintf(stdout, "%s\n", strerror(errno));
			break;
		}
	}
	while (!term);

	free(ptr);

	return ret;
}

int main(int argc, char *argv[])
{
	int ret;
	size_t factor = 1, i = 0, offset;

	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);

	if (1 < argc)
	{
		/* cheap version of getopt(3) */
		if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))
		{
			usage(program_invocation_name);
		}
		factor = (size_t)strtoul(argv[1], NULL, 10);
	}

	if (MAX_FACTOR < factor)
	{
		i = factor;
		factor = MAX_FACTOR;
	}
	else if (!factor)
	{
		i = factor;
		factor = 1;
	}

	if (i)
	{
		fprintf(stdout, "corrected FACTOR from %zu to %zu\n", i, factor);
	}

	offset = (size_t)getpagesize() * factor * 1024;

	ret = memory_hamster(offset);

	return ret;
}
