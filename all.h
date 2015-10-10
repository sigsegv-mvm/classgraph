#ifndef _LIBTF2MOD_ALL_H
#define _LIBTF2MOD_ALL_H


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


/* c standard library */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <link.h>
#include <math.h>

/* posix */
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/user.h>
#include <time.h>

/* glibc */
#include <execinfo.h>
#include <libgen.h>

/* libbsd */
#include <err.h>
#include <bsd/string.h>

/* libdl */
#include <dlfcn.h>

/* libiberty */
#define HAVE_DECL_BASENAME 1
#include <demangle.h>

/* libelf */
#include <libelf.h>
#include <gelf.h>

/* sourcemod */
//#include "sourcemod/asm.h"

/* this project */
#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"
//#include "types.h"
//#include "init.h"
#include "library.h"
//#include "detour.h"
#include "symtab.h"
//#include "vtable.h"
//#include "patch.h"
//#include "convar.h"
//#include "sendprop.h"
//#include "datamap.h"
//#include "symbols.h"
//#include "util.h"
//#include "entprop.h"
//#include "backtrace.h"
#ifdef __cplusplus
}
#endif


#endif
