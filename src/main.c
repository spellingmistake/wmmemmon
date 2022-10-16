/* $Id: main.c,v 1.1.1.1 2002/10/14 12:23:26 sch Exp $ */

/*
 *    WMMemMon - Window Maker memory/swap monitor dockapp
 *
 *    Copyright (c) 2001,2002  Seiichi SATO <ssato@sh.rim.or.jp>
 *    Copyright (c) 2002       Michal Kowalczuk <sammael@brzydal.eu.org>

 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.

 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.

 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dockapp.h"
#include "mem.h"
#include "backdrop_on.xpm"
#include "backdrop_off.xpm"
#include "parts_mem.xpm"
#include "parts_swap.xpm"

#define SIZE 58
#define WINDOWED_BG "  \tc #AEAAAE"

typedef enum { LIGHTON, LIGHTOFF } light;

static Pixmap pixmap;
static Pixmap backdrop_on;
static Pixmap backdrop_off;
static Pixmap parts_mem;
static Pixmap parts_swap;
static Pixmap mask;
static char	*display_name = "";
static char	*title = NULL;
static char	*light_color = NULL;
static unsigned	update_interval = 1;
static light	backlight = LIGHTOFF;
static int 	alarm_mem = 101;	/* specified value 0 - 100 */
static int	alarm_swap = 101;	/* specified value 0 - 100 */

static struct	mem_options mem_opts;	/* options for mem_<ARCH>.c  */

/* prototypes */
static void switch_light(void);		/* switch light without drawing */
static void draw(int mem, int swap);
static void parse_arguments(int argc, char **argv);
static void print_help(char *prog);

int
main(int argc, char **argv)
{
    XEvent event;
    light pre_backlight;
    int usage_mem, usage_swap;
    int i, isalarm = 0;
    int ncolor = 0;
    XpmColorSymbol colors[6] = { {"Back0", NULL, 0},  {"Back1", NULL, 0},
				 {"Back2", NULL, 0},  {"Back3", NULL, 0},
				 {"Black1", NULL, 0}, {"Black2", NULL, 0} };

    /* Parse CommandLine */
    mem_opts.ignore_buffers = mem_opts.ignore_cached
			    = mem_opts.ignore_wired
			    = False;
    parse_arguments(argc, argv);

    /* Initialize Application */
    mem_init();
    dockapp_open_window(display_name, title == NULL ? PACKAGE : title,
			SIZE, SIZE, argc, argv);
    dockapp_set_eventmask(ButtonPressMask);

    if (light_color) {
	colors[0].pixel = dockapp_getcolor_pixel(light_color);
	colors[1].pixel = dockapp_blendedcolor(light_color, -8, -8, -8,1.0000);
	colors[2].pixel = dockapp_blendedcolor(light_color,-16,-16,-16,1.0000);
	colors[3].pixel = dockapp_blendedcolor(light_color,-24,-24,-24,1.0000);
	colors[4].pixel = dockapp_blendedcolor(light_color,  0,  0,  0,0.6666);
	colors[5].pixel = dockapp_blendedcolor(light_color,  0,  0,  0,0.3333);
	ncolor = 6;
    }

    
    /* change raw xpm data to pixmap */
    if (dockapp_stat == WINDOWED_WITH_PANEL)
	backdrop_on_xpm[1] = backdrop_off_xpm[1] = WINDOWED_BG;
    dockapp_xpm2pixmap(backdrop_on_xpm, &backdrop_on, &mask, colors, ncolor);
    dockapp_xpm2pixmap(backdrop_off_xpm, &backdrop_off, NULL, NULL, 0);
    dockapp_xpm2pixmap(parts_mem_xpm, &parts_mem, NULL, colors, ncolor);
    dockapp_xpm2pixmap(parts_swap_xpm, &parts_swap, NULL, colors, ncolor);
    /* shape window */
    if (dockapp_stat == DOCKABLE_ICON || dockapp_stat == WINDOWED) {
	dockapp_setshape(mask, 0, 0);
    }
    if (mask) {
	XFreePixmap(display, mask);
    }
    /* pixmap : draw area */
    pixmap = dockapp_XCreatePixmap(SIZE, SIZE);


    /* Initialize pixmap */
    draw(0, 0);
    dockapp_set_background(pixmap);
    dockapp_show();


    /* Splash */
    pre_backlight = backlight;
    backlight = LIGHTON;
    for (i = 0; i <= 20; i++) {
	if (i <= 10) {	    /* outside */
	    draw(i * 10, 0);
	} else {	    /* inside */
	    draw(100, (i - 10) * 10);
	}
	XSync(display, True);
	if (dockapp_nextevent_or_timeout(&event, 80) == True)
	    break;
    }
    backlight = pre_backlight;


    /* Main Loop */
    for (;;) {
	if (dockapp_nextevent_or_timeout(&event, update_interval * 1000)) {
	/* Next Event */
	    switch(event.type) {
		case ButtonPress:
		    switch_light();
		    draw(usage_mem, usage_swap);
		    break;
		default: /* make gcc happy */
		    break;
	    }
	} else {
	/* Time Out */

	    /* get current usage */
	    mem_getusage(&usage_mem, &usage_swap, &mem_opts);
	    
	    /* alarm? */
	    if (usage_mem >= alarm_mem || usage_swap >= alarm_swap) {
		if (!isalarm) {
		    pre_backlight = backlight;
		}
		isalarm = 1;
	    } else if (isalarm) {
		backlight = pre_backlight;
		isalarm = 0;
	    } else {
		isalarm = 0;
	    }
	    if (isalarm && (backlight == LIGHTOFF)) {
		switch_light();
	    }

	    /* draw */
	    draw(usage_mem, usage_swap);
	}
    }
    return 0;
}


static void
switch_light()
{
    switch (backlight) {
	case LIGHTOFF:
	    backlight = LIGHTON;
	    break;
	case LIGHTON:
	    backlight = LIGHTOFF;
	    break;
    }
}


static void
draw(int mem, int swap)
{
    int ym = 0;
    int ys = 0;

    mem /= 10;
    swap /= 10;
    if (mem > 10) {
	mem = 10;
    }

    /* clear */
    switch (backlight) {
	case LIGHTON:
	    dockapp_copyarea(backdrop_on, pixmap, 0, 0, SIZE, SIZE, 0, 0);
	    ym = 40;
	    ys = 21;
	    break;
	case LIGHTOFF:
	    dockapp_copyarea(backdrop_off, pixmap, 0, 0, SIZE, SIZE, 0, 0);
	    break;
    }

    /* draw memory usage - outside */
    if (mem != 0 && mem < 5)
	dockapp_copyarea(parts_mem, pixmap, (mem - 1) * 19, ym, 19, 40, 30, 9);
    if (mem >= 5)
	dockapp_copyarea(parts_mem, pixmap, 76, ym, 19, 40, 30, 9);
    if (mem > 5)
	dockapp_copyarea(parts_mem, pixmap, 95+(mem-6)*20, ym, 20, 40, 9, 9);

    /* draw swap usage - inside */
    if (swap != 0 && swap < 5) {
	dockapp_copyarea(parts_swap, pixmap,   (swap-1)*10,  ys, 4,21, 30,19);
	dockapp_copyarea(parts_swap, pixmap, 4+(swap-1)*10,ys+2, 3,16, 34,21);
	dockapp_copyarea(parts_swap, pixmap, 7+(swap-1)*10,ys+4, 2,13, 37,23);
	dockapp_copyarea(parts_swap, pixmap, 9+(swap-1)*10,ys+8, 1, 4, 39,27);
    }
    if (swap >= 5) {
	dockapp_copyarea(parts_swap, pixmap, 40, ys,   4,21, 30,19);
	dockapp_copyarea(parts_swap, pixmap, 44, ys+2, 3,16, 34,21);
	dockapp_copyarea(parts_swap, pixmap, 47, ys+4, 2,13, 37,23);
	dockapp_copyarea(parts_swap, pixmap, 49, ys+8, 1, 4, 39,27);
    }
    if (swap > 5) {
	dockapp_copyarea(parts_swap, pixmap, 50+(swap-6)*10, ys+5, 2,11, 19,24);
	dockapp_copyarea(parts_swap, pixmap, 52+(swap-6)*10, ys+2, 2,17, 21,21);
	dockapp_copyarea(parts_swap, pixmap, 54+(swap-6)*10, ys+1, 2,19, 23,20);
	dockapp_copyarea(parts_swap, pixmap, 56+(swap-6)*10,   ys, 4,21, 25,19);
    }

    /* copy to window */
    dockapp_copy2window(pixmap);
}


static void
parse_arguments(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; i++) {
	if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
	    print_help(argv[0]), exit(0);

	else if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v"))
	    printf("%s version %s\n", PACKAGE, VERSION), exit(0);

	else if (!strcmp(argv[i], "--display") || !strcmp(argv[i], "-d")) {
	    display_name = argv[i + 1];
	    i++;
	}
        
	else if (!strcmp(argv[i], "--alarm-mem") || !strcmp(argv[i], "-am")) {
	    int integer;
	    if (argc == i + 1)
		alarm_mem = 90;
	    else if (sscanf(argv[i + 1], "%i", &integer) != 1)
		alarm_mem = 90;
	    else if (integer < 0 || integer > 100)
		fprintf(stderr, "%s: argument %s must be from 0 to 100\n",
			argv[0], argv[i]), exit(1);
	    else
		alarm_mem = integer, i++;
	}

	else if (!strcmp(argv[i], "--alarm-swap") || !strcmp(argv[i], "-as")) {
	    int integer;
	    if (argc == i + 1)
		alarm_swap = 50;
	    else if (sscanf(argv[i + 1], "%i", &integer) != 1)
		alarm_swap = 50;
	    else if (integer < 0 || integer > 100)
		fprintf(stderr, "%s: argument %s must be from 0 to 100\n",
			argv[0], argv[i]), exit(1);
	    else
		alarm_swap = integer, i++;
	}
	
	else if (!strcmp(argv[i], "--backlight") || !strcmp(argv[i], "-bl"))
	    backlight = LIGHTON;

	else if (!strcmp(argv[i], "--light-color") || !strcmp(argv[i], "-lc")) {
	    light_color = argv[i + 1];
	    i++;
	}

	else if (!strcmp(argv[i], "--interval") || !strcmp(argv[i], "-i")) {
	    int integer;
	    if (argc == i + 1)
		fprintf(stderr,
			"%s: error parsing argument for option %s\n",
			argv[0], argv[i]), exit(1);
	    if (sscanf(argv[i + 1], "%i", &integer) != 1)
		fprintf(stderr,
			"%s: error parsing argument for option %s\n",
			argv[0], argv[i]), exit(1);
	    if (integer < 1)
		fprintf(stderr, "%s: argument %s must be >=1\n",
			argv[0], argv[i]), exit(1);
	    update_interval = integer;
	    i++;
	}

	else if (!strcmp(argv[i], "--windowed") || !strcmp(argv[i], "-w"))
	    dockapp_stat = WINDOWED;

	else if (!strcmp(argv[i], "--windowed-withpanel")
		   || !strcmp(argv[i], "-wp"))
	    dockapp_stat = WINDOWED_WITH_PANEL;

	else if (!strcmp(argv[i], "--broken-wm") || !strcmp(argv[i], "-bw"))
	    dockapp_isbrokenwm = True;

	else if (!strcmp(argv[i], "--title") || !strcmp(argv[i], "-t")) {
	    title = argv[i + 1];
	    i++;
	}

#ifdef IGNORE_BUFFERS
	else if (!strcmp(argv[i], "--ignore-buffers") || !strcmp(argv[i], "-b"))
	    mem_opts.ignore_buffers = True;
#endif
#ifdef IGNORE_CACHED
	else if (!strcmp(argv[i], "--ignore-cached") || !strcmp(argv[i], "-c"))
	    mem_opts.ignore_cached = True;
#endif
#ifdef IGNORE_WIRED
	else if (!strcmp(argv[i], "--ignore-wired") || !strcmp(argv[i], "-wr"))
	    mem_opts.ignore_wired = True;
#endif


	else {
	    fprintf(stderr, "%s: unrecognized option '%s'\n", argv[0],
		    argv[i]);
	    print_help(argv[0]), exit(1);
	}
    }
    if (alarm_mem != 101 && alarm_swap != 101) { 
	fprintf(stderr,
	    "%s: select either '-am, --alarm-mem' or '-as, --alarm-swap'\n",
	    argv[0]);
	exit(1);
    }

}

static void
print_help(char *prog)
{
    printf("Usage : %s [OPTIONS]\n", prog);
    printf("WMMemMon - Window Maker memory/swap monitor dockapp\n");
    printf("  -d,  --display <string>        display to use\n");
    printf("  -t,  --title <string>          application title name\n");
    printf("  -bl, --backlight               turn on back-light\n");
    printf("  -lc, --light-color <string>    back-light color(rgb:6E/C6/3B is default)\n");
    printf("  -i,  --interval <number>       number of secs between updates (1 is default)\n");
#ifdef IGNORE_BUFFERS
    printf("  -b,  --ignore-buffers          ignore buffers\n");
#endif
#ifdef IGNORE_CACHED
    printf("  -c,  --ignore-cached           ignore cached pages\n");
#endif
#ifdef IGNORE_WIRED
    printf("  -wr, --ignore-wired            ignore wired pages\n");
#endif
    printf("  -h,  --help                    show this help text and exit\n");
    printf("  -v,  --version                 show program version and exit\n");
    printf("  -w,  --windowed                run the application in windowed mode\n");
    printf("  -wp, --windowed-withpanel      run the application in windowed mode\n");
    printf("                                 with background panel\n");
    printf("  -bw, --broken-wm               activate broken window manager fix\n");
    printf("  -am, --alarm-mem <percentage>  activate alarm mode of memory. <percentage>\n");
    printf("                                 is threshold of percentage from 0 to 100.\n");
    printf("                                 (90 is default)\n");
    printf("  -as, --alarm-swap <percentage> activate alarm mode of swap. <percentage> is\n");
    printf("                                 threshold of percentage from 0 to 100.\n");
    printf("                                 (50 is default)\n");
}
