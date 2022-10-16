bin_PROGRAMS = wmmemmon

wmmemmon_DATA = $(XPMS)

wmmemmondir = ${datarootdir}

XPMS = \
	xpms/backdrop_off.xpm xpms/backdrop_on.xpm \
	xpms/parts_mem.xpm xpms/parts_swap.xpm

wmmemmon_SOURCES = \
	main.c mem.h platforms/mem_$(OS).c $(XPMS)

wmmemmon_CPPFLAGS = \
	-DPIXMAP_DIR=\"${datarootdir}\"

AM_CFLAGS = \
	-Ixpms

INSTALL_PROGRAM = \
	@INSTALL_PROGRAM@ \
	@SETGID_FLAGS@ \
	$(AM_INSTALL_PROGRAM_FLAGS)

EXTRA_DIST = $(XPMS)
AUTOMAKE_OPTIONS = subdir-objects
MAINTAINERCLEANFILES = Makefile.in *.Tpo
