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

/* c++ standard library */
#ifdef __cplusplus
#include <climits>
#include <regex>
#include <set>
#include <map>
#endif

/* libboost */
#ifdef __cplusplus
#include <boost/algorithm/string/join.hpp>
#endif

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
#include <libiberty/demangle.h>

/* libelf */
#include <libelf.h>
#include <gelf.h>

/* apple osx */
#define __LITTLE_ENDIAN__
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/reloc.h>
#include <mach-o/fat.h>

/* otool */
#ifdef __cplusplus
extern "C" {
#endif
#include "otool/dyld_bind_info.h"
#ifdef __cplusplus
}
#endif

/* this project */
#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"
#include "library.h"
#include "symtab.h"
#include "demangle.h"
#include "symtab_elf.h"
#include "symtab_macho.h"
#ifdef __cplusplus
}
#endif


#endif
