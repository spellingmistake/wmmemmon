/*
 * WMMemMon - Window Maker memory/swap monitor dockapp
 *
 * Copyright (c) 2001,2002  Seiichi SATO <ssato@sh.rim.or.jp>
 * Copyright (c) 2002       Michal Kowalczuk <sammael@brzydal.eu.org>
 * Copyright (c) 2022       Thomas Egerer <thomas.egerer@port22.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libdockapp/dockapp.h>

#include "mem.h"

#include "backdrop_on.xpm"
#include "backdrop_off.xpm"
#include "parts_mem.xpm"
#include "parts_swap.xpm"

#define SIZE 58

static int finalize;

typedef struct config_t config_t;

struct config_t {
	char *display_name;
	int lit;
	size_t update_interval;
	size_t alarm_mem;
	size_t alarm_swap;
	mem_options_t mem_opts;
};

enum pixmap_type_t {
	BACK_OFF,
	BACK_ON,
	PRTS_MEM,
	PRTS_SWP,
	CANVAS,
	PIXMAP_MAX,
};

static void sighandler(int signal __attribute__((unused)))
{
	finalize = 1;
}

__NORET static void usage(char *prog, int code)
{
	fprintf(stdout, "Usage : %s [OPTIONS]\n"
		"%s - Window Maker memory/swap monitor dockapp\n"
		"  -d,  --display <string>        display to use\n"
		"  -b,  --backlight               turn on back-light\n"
		"  -i,  --interval <number>       number of secs between updates (1 is default)\n"
		IGNORE_BUFFERS_HELPTEXT IGNORE_CACHED_HELPTEXT IGNORE_WIRED_HELPTEXT
		"  -h,  --help                    show this help text and exit\n"
		"  -v,  --version                 show program version and exit\n"
		"  -m,  --alarm-mem <percentage>  activate alarm mode of memory. <percentage>\n"
		"                                 is threshold of percentage from 0 to 100.\n"
		"                                 (90 is default)\n"
		"  -s,  --alarm-swap <percentage> activate alarm mode of swap. <percentage> is\n"
		"                                 threshold of percentage from 0 to 100.\n"
		"                                 (50 is default)\n", prog, PACKAGE);
	exit(code);
}

static struct option longopts[] = {
	{ "display",    required_argument, 0, 'd'},
	{ "backlight",  no_argument,       0, 'b'},
	{ "interval",   required_argument, 0, 'i'},
	{ "help",       no_argument,       0, 'h'},
	{ "version",    no_argument,       0, 'v'},
	{ "alarm-mem",  required_argument, 0, 'm'},
	{ "alarm-swap", required_argument, 0, 's'},
	IGNORE_BUFFERS_OPTION IGNORE_CACHED_OPTION IGNORE_WIRED_OPTION
	{ 0 },
};

static void arg2size_t(char c, char *optarg , size_t *ptr)
{
	*ptr = (size_t)strtoul(optarg, NULL, 10);
}

static void parse_arguments(int argc, char *argv[], config_t *config)
{
	int c;

	do {
		c = getopt_long(argc, argv, "d:bi:hvm:s:" OPTSTRING, longopts, NULL);

		switch (c)
		{
			case 'd':
				config->display_name = optarg;
				break;
			case 'b':
				config->lit = 1;
				break;
			case 'i':
				arg2size_t(c, optarg, &config->update_interval);
				if (0 == config->update_interval)
				{
					config->update_interval = 1;
					fprintf(stderr, "correcting update_interval from 0 to "
							"%zu\n", config->update_interval);
				}
				break;
			case 'h':
				usage(program_invocation_name, 0);
				break;
			case 'v':
				printf("%s version %s\n", PACKAGE, VERSION);
				exit(0);
				break;
			case 'm':
				arg2size_t(c, optarg, &config->alarm_mem);
				break;
			case 's':
				arg2size_t(c, optarg, &config->alarm_swap);
				break;
			/* the --ignore-<something> options can appear outside an ugly
			 * ifdef; if they're not present in the optstring the case
			 * statements aren't triggered anyways; and since optstring is
			 * autotooled, we should be fine */
			case SHORT_OPTION_IGNORE_BUFFERS:
				config->mem_opts.ignore_buffers = true;
				break;
			case SHORT_OPTION_IGNORE_CACHED:
				config->mem_opts.ignore_cached = true;
				break;
			case SHORT_OPTION_IGNORE_WIRED:
				config->mem_opts.ignore_wired = true;
				break;
			case -1:
				break;
			default:
				usage(program_invocation_name, -1);
				break;
		}
	}
	while (-1 != c);

	if (101 != config->alarm_mem && 101 != config->alarm_swap)
	{
		fprintf(stderr, "%s: select either '-m, --alarm-mem' or '-s, "
				"--alarm-swap'\n", argv[0]);
		exit(-1);
	}
}

/**
 * pixmaps: backdrop_on, backdrop_off, memory, swap, canvas
 */
static void draw(DAShapedPixmap *pixmaps[], int memory[], int lit)
{
	int ym, ys, usage_mem, usage_swp;

	usage_mem = memory[0];
	usage_swp = memory[1];

	usage_mem /= 10;
	usage_swp /= 10;

	if (usage_mem > 10)
	{
		usage_mem = 10;
	}

	if (lit)
	{
		ym = 40;
		ys = 21;
	}
	else
	{
		ym = ys = 0;
	}

	DASPCopyArea(pixmaps[lit], pixmaps[CANVAS], 0, 0, SIZE, SIZE, 0, 0);

	/* draw memory usage - outside */
	if (usage_mem != 0 && usage_mem < 5)
	{
		DASPCopyArea(pixmaps[PRTS_MEM], pixmaps[CANVAS],
				(usage_mem - 1) * 19, ym, 19, 40, 30, 9);
	}
	if (usage_mem >= 5)
	{
		DASPCopyArea(pixmaps[PRTS_MEM], pixmaps[CANVAS],
				76, ym, 19, 40, 30, 9);
	}
	if (usage_mem > 5)
	{
		DASPCopyArea(pixmaps[PRTS_MEM], pixmaps[CANVAS],
				95 + (usage_mem - 6) * 20, ym, 20, 40, 9, 9);
	}

	/* draw usage_swp usage - inside */
	if (usage_swp != 0 && usage_swp < 5)
	{
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				(usage_swp - 1) * 10, ys, 4, 21, 30,19);
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				4 + (usage_swp - 1) * 10, ys + 2, 3, 16, 34, 21);
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				7 + (usage_swp - 1) * 10, ys + 4, 2, 13, 37, 23);
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				9 + (usage_swp - 1) * 10, ys + 8, 1, 4, 39, 27);
	}
	if (usage_swp >= 5)
	{
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				40, ys, 4,21, 30,19);
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				44, ys + 2, 3, 16, 34, 21);
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				47, ys + 4, 2, 13, 37, 23);
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				49, ys + 8, 1, 4, 39, 27);
	}
	if (usage_swp > 5)
	{
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				50 + (usage_swp - 6) * 10, ys + 5, 2, 11, 19, 24);
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				52 + (usage_swp - 6) * 10, ys + 2, 2, 17, 21, 21);
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				54 + (usage_swp - 6) * 10, ys + 1, 2, 19, 23, 20);
		DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
				56 + (usage_swp - 6) * 10, ys, 4, 21, 25, 19);
	}

	XCopyArea(DAGetDisplay(NULL), pixmaps[CANVAS]->pixmap, DAWindow, DAGC, 0, 0,
			SIZE, SIZE, 0, 0);
}

int main(int argc, char *argv[])
{
	XEvent event;
	DAShapedPixmap *pixmaps[PIXMAP_MAX];
	int memory_usage[2] = { 0 }, i, isalarm = 0, was_lit;
	config_t config = {
		.update_interval = 1,
		.alarm_mem = 101,
		.alarm_swap = 101,
	};

	parse_arguments(argc, argv, &config);

	DASetExpectedVersion(20030126);
	DAOpenDisplay(config.display_name, argc, argv);
	DACreateIcon(PACKAGE, SIZE, SIZE, argc, argv);

	pixmaps[BACK_OFF] = DAMakeShapedPixmapFromData(backdrop_off_xpm);
	pixmaps[BACK_ON] = DAMakeShapedPixmapFromData(backdrop_on_xpm);
	pixmaps[PRTS_MEM] = DAMakeShapedPixmapFromData(parts_mem_xpm);
	pixmaps[PRTS_SWP] = DAMakeShapedPixmapFromData(parts_swap_xpm);
	pixmaps[CANVAS] = DAMakeShapedPixmap();

	DASetShapeWithOffsetForWindow(DAWindow, pixmaps[BACK_OFF]->shape, 0, 0);
	draw(pixmaps, memory_usage, config.lit);
	DASetPixmap(pixmaps[CANVAS]->pixmap);
	DAShow();

	/* Splash */
	for (i = 0; i <= 20; i++)
	{
		if (i <= 10)
		{	/* outside */
			memory_usage[0] = i * 10;
			memory_usage[1] = 0;
		}
		else
		{	/* inside */
			memory_usage[0] = 100;
			memory_usage[1] = (i - 10) * 10;
		}
		draw(pixmaps, memory_usage, config.lit);
		if (DANextEventOrTimeout(&event, 80))
		{
			break;
		}
	}

	XSelectInput(DAGetDisplay(NULL), DAWindow, ButtonPressMask);

	while (!finalize)
	{
		if (DANextEventOrTimeout(&event, config.update_interval * 1000))
		{
			switch(event.type)
			{
				case ButtonPress:
					config.lit = ++(config.lit) % 2;
					break;
				default:
					continue;
			}
		}
		else
		{
			mem_getusage(memory_usage, &config.mem_opts);

			if (!isalarm &&
				(memory_usage[0] >= config.alarm_mem ||
				 memory_usage[1] >= config.alarm_swap))
			{
				was_lit = config.lit;
				isalarm = 1;
				config.lit = ++(config.lit) % 2;
			}
			else
			{
				if (isalarm)
				{
					config.lit = was_lit;
				}
				isalarm = 0;
			}
		}

		draw(pixmaps, memory_usage, config.lit);
	}

	for (i = 0; i < (sizeof(pixmaps) / sizeof(pixmaps[BACK_ON])); ++i)
	{
		if (pixmaps[i])
		{
			DAFreeShapedPixmap(pixmaps[i]);
		}
	}

	return 0;
}
