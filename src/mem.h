#ifndef __MEM_H_
#define __MEM_H_

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#if HAVE_STDINT_H
# include <stdint.h>
#else          /* #if HAVE_STDINT_H */
#define u_int64_t long long unsigned int;
#endif         /* #if HAVE_STDINT_H */

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else          /* #if HAVE_INTTYPES_H */
#define PRIu64 llu
#endif         /* #if HAVE_INTTYPES_H */

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else          /* #ifdef HAVE_STDBOOL_H */
# ifndef HAVE__BOOL
#   define _Bool signed char
# endif                /* # ifndef HAVE__BOOL */
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif         /* #ifdef HAVE_STDBOOL_H */

#ifndef __UNUSED
# define __UNUSED __attribute__((unused))
#endif /* #ifndef __UNUSED */

#ifndef __CTOR
# define __CTOR __attribute__((constructor))
#endif /* #ifndef __CTOR */

#ifndef __NORET
# define __NORET __attribute__((noreturn))
#endif	/* #ifndef _NORET */

typedef struct mem_options_t mem_options_t;

/**
 * memory options for various platforms (that I don't know)
 */
struct mem_options_t
{
	/** ignore buffers */
	bool ignore_buffers;
	/** ignore cached pages */
	bool ignore_cached;
	/** ignore wired pages */
	bool ignore_wired;
};

/**
 * calculate the memory usage of the system (main memory and swap sapce)
 * in percent [0; 100] and place them into \c memory_usage (array index 0
 * main memory, array index 1 swap space)
 *
 * @param memory_usage pointer to array to hold memory usage information
 * @param opts                 options to use when obtaining memory usage
 */
// TODO: return boolean to signify an error and change the appearance!
void mem_getusage(int *memory_usage, const mem_options_t *opts);

#endif /* #ifndef __MEM_H_ */

