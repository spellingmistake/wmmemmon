AC_PREREQ(2.60)

AC_INIT([wmmemmon],[1.2.1])
AC_CANONICAL_TARGET
AC_ARG_PROGRAM
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR(src/main.c)
AM_CONFIG_HEADER(config.h)

# checks for programs
AC_PROG_LN_S
AC_PROG_INSTALL

# set up CFLAGS *before* AC_PROG_CC, otherwise is enables '-g -O2'
if test x$enable_debug = xyes; then
	CFLAGS="$CFLAGS -ggdb -O0"
	AC_DEFINE(DEBUG, 1, [use debug code])
else
	CFLAGS="$CFLAGS -O3"
fi

CFLAGS+=" -Wall -Wextra -Werror"

AC_PROG_CC

# use _GNU_SOURCE
AC_GNU_SOURCE

m4_include(m4/macros.m4)

# configure optinons and paths to look for libraries and headers
AX_CONFIG_DSBL([contrib],         [don not build contrib files (default: on])
AX_CONFIG_ENBL([debug],           [turn on debugging (default: off)])

# libdockapp should suffice, as it links against Xext, Xpm and X11
AC_SEARCH_LIBS([DAInitialize], [dockapp], [have_libdockap=yes], [AC_MSG_ERROR([required dockapp library not found])])
AC_SEARCH_LIBS([XCopyArea], [X11], [have_x11=yes], [AC_MSG_ERROR([required X11 library not found])])

AC_CHECK_HEADER([stdint.h],, [AC_MSG_ERROR([required include file stdint.h not found])])
AC_CHECK_HEADER([inttypes.h],, [AC_MSG_ERROR([required include file inttypes.h not found])])
AC_CHECK_HEADER([getopt.h],, [AC_MSG_ERROR([required include file getopt.h not found])])
AC_CHECK_HEADER([libdockapp/dockapp.h],, [AC_MSG_ERROR([required include file dockapp.h not found])])

# check for header files, types and functions
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# check for OS
case ${target_os} in
linux*)
	OS=linux
	ignore_buffers=yes
	ignore_cached=yes
	;;
freebsd*)
	OS=freebsd
	ignore_wired=yes
	ignore_cached=yes
	LIBS="$LIBS -lkvm"
	SETGID_FLAGS="-g kmem -m 2755 -o root"
	;;
\(open|net\)bsd*)
	OS=opennetbsd
	;;
solaris*)
	OS=solaris
	;;
cygwin*)
	OS=cygwin
	;;
irix*)
	OS=irix
	;;
darwin*)
	OS=darwin
	;;
*)
	echo -e "\nSorry, ${target_os} is not supported yet.\n\n"
	exit 1
	;;
esac

AC_SUBST(CFLAGS)
AC_SUBST(OS)
AC_SUBST(LIBS)
AC_SUBST(SETGID_FLAGS)

AX_DEFINE_HELPTEXT_AND_OPTION([buffers], [u])
AX_DEFINE_HELPTEXT_AND_OPTION([cached ], [c])
AX_DEFINE_HELPTEXT_AND_OPTION([wired  ], [e])

AC_DEFINE_UNQUOTED(OPTSTRING, ["$OPTSTRING"], [additional values for getopt_long(3) optstring])

AX_PROGRAM_INVOCATION_NAME

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	doc/Makefile
	contrib/Makefile
	contrib/wmmemmon.spec])

AC_OUTPUT
