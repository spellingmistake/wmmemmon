# $Id: Makefile.am,v 1.2 2002/12/08 17:11:23 sch Exp $

bin_PROGRAMS = wmmemmon

wmmemmon_SOURCES =   \
	main.c \
	mem.h \
	mem_@OS@.c \
	backdrop_off.xpm \
	backdrop_on.xpm \
	parts_mem.xpm \
	parts_swap.xpm

EXTRA_wmmemmon_SOURCES = \
	mem_linux.c \
	mem_freebsd.c \
	mem_freebsd_sysctl.c \
	mem_openbsd.c \
	mem_netbsd.c \
	mem_solaris.c \
	mem_cygwin.c \
	mem_irix.c \
	mem_darwin.c

wmmemmon_DEPENDENCIES = ../libdockapp/dockapp.o

INCLUDES = \
	@HEADER_SEARCH_PATH@
	@XCFLAGS@

CFLAGS = @DFLAGS@ @CFLAGS@ -I../libdockapp/

wmmemmon_LDADD = \
	dockapp.o \
	@LIBRARY_SEARCH_PATH@ \
	@XLIBS@
	@XLFLAGS@

INSTALL_PROGRAM = \
	@INSTALL_PROGRAM@ \
	@SETGID_FLAGS@ \
	$(AM_INSTALL_PROGRAM_FLAGS)
