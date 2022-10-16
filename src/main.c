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

#define CANVAS_SIZE 58

/* default resolution */
#define RESOLUTION  10

#define countof(x) (sizeof((x)) / sizeof((x)[0]))

static int finalize;

typedef enum pixmap_type_t pixmap_type_t;

enum pixmap_type_t {
	PM_TYPE_BACK_ON,
	PM_TYPE_BACK_OFF,
	PM_TYPE_PARTS_MEM,
	PM_TYPE_PARTS_SWP,
	PM_TYPE_CANVAS,
	PM_TYPE_MAX,
};

typedef struct config_t config_t;
typedef struct offset_table_t offset_table_t;
typedef struct pixmap_coords_t pixmap_coords_t;

struct config_t {
	char *display_name;
	int lit;
	size_t update_interval;
	size_t alarm_mem;
	size_t alarm_swap;
	size_t resolution;
	size_t steps;
	mem_options_t mem_opts;
	const char *pixmap_names[PM_TYPE_MAX];
	const char *pixmap_path;
};

struct pixmap_coords_t {
	size_t height;
	size_t width;
	size_t x_left;
	size_t x_rght;
	size_t y;
	bool use_resolution;
};

struct offset_table_t {
	int width;
	int y_offset;
};

static pixmap_coords_t coords[] = {
	{
		.height = 44,
		.width = 22,
		.x_left = 7,
		.x_rght = 29,
		.y = 7,
		.use_resolution = true,
	},
	{
		.height = 22,
		.width = 11,
		.x_left = 18,
		.x_rght = 29,
		.y = 18,
		.use_resolution = true,
	},
	{
		.height = CANVAS_SIZE,
		.width = CANVAS_SIZE,
	},
};

static pixmap_coords_t *get_pixmap_coords(pixmap_type_t type)
{
	pixmap_coords_t *ret;

	switch (type)
	{
		case PM_TYPE_PARTS_MEM:
			ret = &coords[0];
			break;
		case PM_TYPE_PARTS_SWP:
			ret = &coords[1];
			break;
		case PM_TYPE_BACK_ON:
		case PM_TYPE_BACK_OFF:
		case PM_TYPE_CANVAS:
			ret = &coords[2];
			break;
		default:
			fprintf(stdout, "unable to determine pixmap coordinates for "
					"type %u\n", type);
			abort();
			ret = NULL;
			break;
	}

	return ret;
}

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
		"  -i,  --interval <number>       number of secs between updates (default 1)\n"
		IGNORE_BUFFERS_HELPTEXT IGNORE_CACHED_HELPTEXT IGNORE_WIRED_HELPTEXT
		"  -h,  --help                    show this help text and exit\n"
		"  -v,  --version                 show program version and exit\n"
		"  -p,  --pixmap-path <path>      alternative path to pixmap files\n"
		"  -r,  --resolution <number>     memory resolution steps (default 10)\n"
		"                                 Note: pixmaps must match resolution\n"
		"  -m,  --alarm-mem <percentage>  activate alarm mode of memory. <percentage>\n"
		"                                 is threshold of percentage from 0 to 100.\n"
		"                                 (default off)\n"
		"  -s,  --alarm-swap <percentage> activate alarm mode of swap. <percentage> is\n"
		"                                 threshold of percentage from 0 to 100.\n"
		"                                 (default off)\n",
		prog, PACKAGE);
	exit(code);
}

static struct option longopts[] = {
	{ "display",    required_argument, 0, 'd'},
	{ "backlight",  no_argument,       0, 'b'},
	{ "interval",   required_argument, 0, 'i'},
	{ "help",       no_argument,       0, 'h'},
	{ "version",    no_argument,       0, 'v'},
	{ "pixmap-path",required_argument, 0, 'p'},
	{ "resolution", required_argument, 0, 'r'},
	{ "alarm-mem",  required_argument, 0, 'm'},
	{ "alarm-swap", required_argument, 0, 's'},
	IGNORE_BUFFERS_OPTION IGNORE_CACHED_OPTION IGNORE_WIRED_OPTION
	{ 0 },
};

static void arg2size_t(char *optarg , size_t *ptr)
{
	*ptr = (size_t)strtoul(optarg, NULL, 10);
}

static void parse_arguments(int argc, char *argv[], config_t *config)
{
	int c;

	do {
		c = getopt_long(argc, argv, "d:bi:hvp:r:m:s:" OPTSTRING, longopts,
				NULL);

		switch (c)
		{
			case 'd':
				config->display_name = optarg;
				break;
			case 'b':
				config->lit = 1;
				break;
			case 'i':
				arg2size_t(optarg, &config->update_interval);
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
			case 'p':
				config->pixmap_path = optarg;
				break;
			case 'r':
				arg2size_t(optarg, &config->resolution);
				if (10 <= config->resolution && 24 >= config->resolution)
				{
					config->steps = (100 / config->resolution);
				}
				else
				{
					fprintf(stderr, "memory resolution must not be "
							"[10; 24]!\n");
					usage(program_invocation_name, -1);
				}
				break;
			case 'm':
				arg2size_t(optarg, &config->alarm_mem);
				break;
			case 's':
				arg2size_t(optarg, &config->alarm_swap);
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
		fprintf(stderr, "select either --alarm-mem or --alarm-swap\n");
		usage(program_invocation_name, -1);
	}
}

static inline void calc_x_offsets(size_t memory, int *xl, int *xr, int width,
		config_t *config)
{
	if (memory > (config->resolution / 2))
	{
		*xl = memory;
		*xr = (config->resolution / 2);
	}
	else
	{
		*xl = 0;
		*xr = memory;
	}

	*xl *= width;
	*xr *= width;
}

static void draw_mem_usage(DAShapedPixmap *pixmaps[], size_t memory[], int lit,
		config_t *config)
{
	int xl, xr;
	pixmap_coords_t *ptr;

	ptr = get_pixmap_coords(PM_TYPE_PARTS_MEM);

	calc_x_offsets(memory[0], &xl, &xr, ptr->width, config);

	/* draw memory usage - outside, right part, 0% - 50% */
	if (xr)
	{
		xr -= ptr->width;
		DASPCopyArea(pixmaps[PM_TYPE_PARTS_MEM], pixmaps[PM_TYPE_CANVAS],
				xr, ptr->height * lit, ptr->width, ptr->height,
				ptr->x_rght, ptr->y);
	}

	/* draw memory usage - outside, right part, 51% - 100% */
	if ((size_t)xl > (5 * ptr->width))
	{
		xl -= ptr->width;
		DASPCopyArea(pixmaps[PM_TYPE_PARTS_MEM], pixmaps[PM_TYPE_CANVAS],
				xl, ptr->height * lit, ptr->width, ptr->height,
				ptr->x_left, ptr->y);
	}
}

static inline void func(DAShapedPixmap *pixmaps[], int x1, int x2, int yo,
		int sign, int lit, int start, int end,
		const offset_table_t *offset_table, pixmap_coords_t *ptr)
{
	int xo, i;

	if (x1)
	{
		xo = 0;
		x1 -= ptr->width;
		i = start;

		do
		{
			DASPCopyArea(pixmaps[PM_TYPE_PARTS_SWP], pixmaps[PM_TYPE_CANVAS],
					x1 + xo, ptr->height * lit + yo,
					offset_table[i].width, ptr->height - 2 * yo,
					x2 + xo, ptr->y + yo);
			xo += offset_table[i].width;
			yo += offset_table[i].y_offset * sign;

			i += sign;
		}
		while (i != end);
	}
}

static void draw_swap_usage(DAShapedPixmap *pixmaps[], size_t memory[], int lit,
		config_t *config)
{
	int xl, xr;
	pixmap_coords_t *ptr;
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

	ptr = get_pixmap_coords(PM_TYPE_PARTS_SWP);
	calc_x_offsets(memory[1], &xl, &xr, ptr->width, config);

	/* draw swap usage - inside, right part, 0% - 50% */
	func(pixmaps, xr, ptr->x_rght, 0, 1, lit, 0, countof(offsets),
			offsets, ptr);

	/* draw swap usage - inside, left part, 51% - 100% */
	func(pixmaps, xl, ptr->x_left, 6, -1, lit, countof(offsets) - 1, 0 - 1,
			offsets, ptr);
}

/**
 * pixmaps: backdrop_on, backdrop_off, memory, swap, canvas
 */
static void draw_canvas(DAShapedPixmap *pixmaps[], const int memory[], int lit,
		config_t *config, pixmap_coords_t *ptr)
{
	size_t mem[2] = {
		memory[0] / config->steps,
		memory[1] / config->steps,
	};

	if (mem[0] > (100 / config->steps))
	{
		mem[0] = (100 / config->steps);
	}

	/* reset canvas with background image */
	DASPCopyArea(pixmaps[lit], pixmaps[PM_TYPE_CANVAS], 0, 0,
			ptr->width, ptr->height, 0, 0);

	draw_mem_usage(pixmaps, mem, lit, config);
	draw_swap_usage(pixmaps, mem, lit, config);

	XCopyArea(DAGetDisplay(NULL), pixmaps[PM_TYPE_CANVAS]->pixmap,
			DAWindow, DAGC, 0, 0, ptr->width, ptr->height, 0, 0);
}

static bool check_resolution(DAShapedPixmap *pixmap, pixmap_type_t type,
		char *file_name, config_t *config)
{
	size_t w, h;
	bool ret = true;
	pixmap_coords_t *ptr;

	ptr = get_pixmap_coords(type);

	if (ptr->use_resolution)
	{
		w = config->resolution * ptr->width;
		h = 2 * ptr->height;
	}
	else
	{
		w = ptr->width;
		h = ptr->width;
	}

	if ((pixmap->geometry.width < w) || (pixmap->geometry.height < h))
	{
		fprintf(stderr, "insufficient pixmap height for '%s', "
				"required %zux%zu, found %dx%d\n", file_name, w, h,
				pixmap->geometry.width, pixmap->geometry.height);
		ret = false;
	}

	return ret;
}

static bool init_pixmaps(DAShapedPixmap *pixmaps[], config_t *config)
{
	char buf[FILENAME_MAX];
	size_t i;
	bool ret = true;

	for (i = 0; i < countof(config->pixmap_names); ++i)
	{
		if (config->pixmap_names[i])
		{
			snprintf(buf, sizeof(buf), "%s/%s", config->pixmap_path,
					config->pixmap_names[i]);
			pixmaps[i] = DAMakeShapedPixmapFromFile(buf);

			if (!pixmaps[i])
			{
				fprintf(stderr, "failed to open pixmap file '%s'\n", buf);
				ret = false;
				break;
			}

			if (!check_resolution(pixmaps[i], i, buf, config))
			{
				ret = false;
				break;
			}
		}
		else
		{
			pixmaps[PM_TYPE_CANVAS] = DAMakeShapedPixmap();
		}
	}

	if (!ret)
	{
		for (i = 0; i < countof(config->pixmap_names); ++i)
		{
			if (pixmaps[i])
			{
				DAFreeShapedPixmap(pixmaps[i]);
			}
		}
	}

	return ret;
}

int main(int argc, char *argv[])
{
	size_t i;
	XEvent event;
	pixmap_coords_t *ptr;
	DAShapedPixmap *pixmaps[PM_TYPE_MAX];
	int memory_usage[2] = { 0 }, isalarm = 0, was_lit;
	config_t config = {
		.update_interval = 1,
		.alarm_mem = 101,
		.alarm_swap = 101,
		.resolution = RESOLUTION,
		.steps = (100 / RESOLUTION),
		.pixmap_names = {
			"backdrop_off.xpm",
			"backdrop_on.xpm",
			"parts_mem.xpm",
			"parts_swap.xpm",
		},
		.pixmap_path = PIXMAP_DIR,
	};

	parse_arguments(argc, argv, &config);
	signal(SIGTERM, sighandler);

	ptr = get_pixmap_coords(PM_TYPE_CANVAS);

	/* to make DAGetDisplay return DADisplay */
	DASetExpectedVersion(20030126);
	DAOpenDisplay(config.display_name, argc, argv);
	DACreateIcon(PACKAGE, ptr->width, ptr->height, argc, argv);

	if (!init_pixmaps(pixmaps, &config))
	{
		usage(program_invocation_name, -1);
	}

	DASetShapeWithOffsetForWindow(DAWindow, pixmaps[PM_TYPE_BACK_ON]->shape,
			0, 0);
	draw_canvas(pixmaps, memory_usage, config.lit, &config, ptr);
	DASetPixmap(pixmaps[PM_TYPE_CANVAS]->pixmap);
	DAShow();

	/* splash */
	for (i = 0; i <= (4 * config.resolution); i++)
	{
		if (i <= config.resolution)
		{	/* increasing memory usage */
			memory_usage[0] = i * config.steps;
			memory_usage[1] = 0;
		}
		else if (i <= (2 * config.resolution))
		{	/* increasing swap usage */
			memory_usage[0] = 100;
			memory_usage[1] = (i - config.resolution) * config.steps;
		}
		else if (i <= (3 * config.resolution))
		{	/* decreasing swap usage */
			memory_usage[0] = 100;
			memory_usage[1] = 100 - ((i - 2 * config.resolution) * config.steps);
		}
		else
		{	/* decreasing memory usage */
			memory_usage[0] = 100 - ((i - 3 * config.resolution) * config.steps);
			memory_usage[1] = 0;
		}
		draw_canvas(pixmaps, memory_usage, config.lit, &config, ptr);
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
					config.lit = (config.lit + 1) % 2;
					break;
				default:
					continue;
			}
		}
		else
		{
			mem_getusage(memory_usage, &config.mem_opts);

			if (!isalarm &&
				((size_t)memory_usage[0] >= config.alarm_mem ||
				 (size_t)memory_usage[1] >= config.alarm_swap))
			{
				was_lit = config.lit;
				isalarm = 1;
				config.lit = (config.lit + 1) % 2;
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

		draw_canvas(pixmaps, memory_usage, config.lit, &config, ptr);
	}

	for (i = 0; i < (sizeof(pixmaps) / sizeof(pixmaps[PM_TYPE_BACK_OFF])); ++i)
	{
		if (pixmaps[i])
		{
			DAFreeShapedPixmap(pixmaps[i]);
		}
	}

	return 0;
}
