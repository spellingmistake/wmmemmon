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

#define CANVAS_SIZE 58

#define MEM_HEIGHT  44
#define MEM_WIDTH   22
#define MEM_LX       7
#define MEM_RX      29
#define MEM_Y        7

#define SWP_HEIGHT  22
#define SWP_WIDTH   11
#define SWP_LX      18
#define SWP_RX      29
#define SWP_Y       18

#define RESOLUTION  10
#define STEPS       (100 / RESOLUTION)

#define countof(x) (sizeof((x)) / sizeof((x)[0]))

static int finalize;

typedef struct config_t config_t;
typedef struct offset_table_t offset_table_t;

struct config_t {
	char *display_name;
	int lit;
	size_t update_interval;
	size_t alarm_mem;
	size_t alarm_swap;
	mem_options_t mem_opts;
};

struct offset_table_t {
	int width;
	int y_offset;
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

static inline void calc_x_offsets(int memory, int *xl, int *xr, int width)
{
	if (memory > (RESOLUTION / 2))
	{
		*xl = memory;
		*xr = (RESOLUTION / 2);
	}
	else
	{
		*xl = 0;
		*xr = memory;
	}

	*xl *= width;
	*xr *= width;
}

static void draw_mem_usage(DAShapedPixmap *pixmaps[], int memory[], int lit)
{
	int xl, xr, xo, yo, i;

	calc_x_offsets(memory[0], &xl, &xr, MEM_WIDTH);

	/* draw memory usage - outside, right part, 0% - 50% */
	if (xr)
	{
		xr -= MEM_WIDTH;
		DASPCopyArea(pixmaps[PRTS_MEM], pixmaps[CANVAS], xr, MEM_HEIGHT * lit,
				MEM_WIDTH, MEM_HEIGHT, MEM_RX, MEM_Y);
	}

	/* draw memory usage - outside, right part, 51% - 100% */
	if (xl > (5 * MEM_WIDTH))
	{
		xl -= MEM_WIDTH;
		DASPCopyArea(pixmaps[PRTS_MEM], pixmaps[CANVAS], xl, MEM_HEIGHT * lit,
				MEM_WIDTH, MEM_HEIGHT, MEM_LX, MEM_Y);
	}
}

static inline void func(DAShapedPixmap *pixmaps[], int x1, int x2, int yo,
		int sign, int lit, int start, int end,
		const offset_table_t *offset_table)
{
	int xo, i;

	if (x1)
	{
		xo = 0;
		x1 -= SWP_WIDTH;
		i = start;

		do
		{
			DASPCopyArea(pixmaps[PRTS_SWP], pixmaps[CANVAS],
					x1 + xo, SWP_HEIGHT * lit + yo,
					offset_table[i].width, SWP_HEIGHT - 2 * yo,
					x2 + xo, SWP_Y + yo);
			xo += offset_table[i].width;
			yo += offset_table[i].y_offset * sign;

			i += sign;
		}
		while (i != end);
	}
}

static void draw_swap_usage(DAShapedPixmap *pixmaps[], int memory[], int lit)
{
	int xl, xr, xo, yo, sign, i;
	// TODO: if we can draw the swap usage at once, this would simplify the
	// code massively
	static const offset_table_t offsets[] = {
		{ 5, 1, },
		{ 1, 1, },
		{ 1, 1, },
		{ 1, 1, },
		{ 1, 1, },
		{ 1, 1, },
		{ 1, 1, },
	};

	calc_x_offsets(memory[1], &xl, &xr, SWP_WIDTH);

	/* draw swap usage - inside, right part, 0% - 50% */
	func(pixmaps, xr, SWP_RX, 0, 1, lit, 0, countof(offsets), offsets);

	/* draw swap usage - inside, left part, 51% - 100% */
	func(pixmaps, xl, SWP_LX, 6, -1, lit, countof(offsets) - 1, 0 - 1, offsets);
}

/**
 * pixmaps: backdrop_on, backdrop_off, memory, swap, canvas
 */
static void draw_canvas(DAShapedPixmap *pixmaps[], const int memory[], int lit)
{
	int xr, xl, xo, yo, i, sign, mem[2] = {
		memory[0] / STEPS,
		memory[1] / STEPS,
	};

	if (mem[0] > (100 / STEPS))
	{
		mem[0] = (100 / STEPS);
	}

	/* reset canvas with background image */
	DASPCopyArea(pixmaps[lit], pixmaps[CANVAS], 0, 0,
			CANVAS_SIZE, CANVAS_SIZE, 0, 0);

	draw_mem_usage(pixmaps, mem, lit);
	draw_swap_usage(pixmaps, mem, lit);

	XCopyArea(DAGetDisplay(NULL), pixmaps[CANVAS]->pixmap, DAWindow, DAGC, 0, 0,
			CANVAS_SIZE, CANVAS_SIZE, 0, 0);
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

	/* to make DAGetDisplay return DADisplay */
	DASetExpectedVersion(20030126);
	DAOpenDisplay(config.display_name, argc, argv);
	DACreateIcon(PACKAGE, CANVAS_SIZE, CANVAS_SIZE, argc, argv);

	pixmaps[BACK_OFF] = DAMakeShapedPixmapFromData(backdrop_off_xpm);
	pixmaps[BACK_ON] = DAMakeShapedPixmapFromData(backdrop_on_xpm);
	pixmaps[PRTS_MEM] = DAMakeShapedPixmapFromData(parts_mem_xpm);
	pixmaps[PRTS_SWP] = DAMakeShapedPixmapFromData(parts_swap_xpm);
	pixmaps[CANVAS] = DAMakeShapedPixmap();

	DASetShapeWithOffsetForWindow(DAWindow, pixmaps[BACK_OFF]->shape, 0, 0);
	draw_canvas(pixmaps, memory_usage, config.lit);
	DASetPixmap(pixmaps[CANVAS]->pixmap);
	DAShow();

	/* Splash */
	for (i = 0; i <= (2 * RESOLUTION); i++)
	{
		if (i <= 10)
		{	/* outside */
			memory_usage[0] = i * STEPS;
			memory_usage[1] = 0;
		}
		else
		{	/* inside */
			memory_usage[0] = 100;
			memory_usage[1] = (i - RESOLUTION) * STEPS;
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

		draw_canvas(pixmaps, memory_usage, config.lit);
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
