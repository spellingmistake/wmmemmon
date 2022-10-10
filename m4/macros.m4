AC_DEFUN([AX_CONFIG_ENBL],
	[AC_ARG_ENABLE(
		[$1],
		AS_HELP_STRING([--enable-$1], [$2]),
		[patsubst([$1], [-], [_])_given=true
		if test x$enableval = xyes; then
			patsubst([$1], [-], [_])=true
		 else
			patsubst([$1], [-], [_])=false
		fi],
		[patsubst([$1], [-], [_])=false
		patsubst([$1], [-], [_])_given=false]
	)]
)

AC_DEFUN([AX_CONFIG_DSBL],
	[AC_ARG_ENABLE(
		[$1],
		AS_HELP_STRING([--disable-$1], [$2]),
		[patsubst([$1], [-], [_])_given=true
		if test x$enableval = xyes; then
			patsubst([$1], [-], [_])=true
		 else
			patsubst([$1], [-], [_])=false
		fi],
		[patsubst([$1], [-], [_])=true
		patsubst([$1], [-], [_])_given=false]
	)]
)

AC_DEFUN([AX_DEFINE_HELPTEXT_AND_OPTION],
	[
		if test x$ignore_$1 = xyes; then
			[IGNORE_]translit([$1], [a-z ], [A-Z])[_HELPTEXT]="  -$2, --ignore-$1           ignore $1 pages\n"
			[IGNORE_]translit([$1], [a-z ], [A-Z])[_OPTION="{ \"ignore-$1\", no_argument, 0, '$2'},"]
			OPTSTRING=${OPTSTRING}[$2]
		else
			IW_OPTION=
		fi
		AC_DEFINE_UNQUOTED(
			[SHORT_OPTION_IGNORE_]translit([$1], [a-z ], [A-Z]),
			[']$2['],
			[short option for '--ignore-$1']
		)
		AC_DEFINE_UNQUOTED(
			[IGNORE_]translit([$1], [a-z ], [A-Z])[_HELPTEXT],
			["$IGNORE_]translit([$1], [a-z ], [A-Z])[_HELPTEXT"],
			[use '--ignore-$1' option]
		)
		AC_DEFINE_UNQUOTED(
			[IGNORE_]translit([$1], [a-z ], [A-Z])[_OPTION],
			[$IGNORE_]translit([$1], [a-z ], [A-Z])[_OPTION],
			[use '--ignore-$1' option]
		)
	]
)

AC_DEFUN([AX_PROGRAM_INVOCATION_NAME],
	AC_MSG_CHECKING([for program_invocation_name])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM(
			[[ #include <errno.h> ]],
			[[ int main(void) { const char *x = program_invocation_name; } ]]
		)],
		[AC_MSG_RESULT([yes])],
		[
			AC_MSG_RESULT([no])
			# m4 quotation is ugly!
			AC_DEFINE([program_invocation_name], [[argv[[0]]]], [program_invocation_name])
		]
	)
)
