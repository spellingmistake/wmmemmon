/* $Id: mem.h,v 1.1.1.1 2002/10/14 12:23:26 sch Exp $ */

/*
 * mem.h - module to get memory/swap usages in percent
 *
 * Copyright (c) 2001 Seiichi SATO <ssato@sh.rim.or.jp>
 *
 * licensed under the GPL
 */

struct mem_options {
    int ignore_buffers;
    int ignore_cached;
    int ignore_wired;
};

void mem_init(void);
void mem_getusage(int *per_mem, int *per_swap, const struct mem_options *opts);
