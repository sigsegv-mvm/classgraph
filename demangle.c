#include "all.h"


const char *try_demangle(const char *mangled)
{
	const char *demangled = cplus_demangle(mangled, DMGL_GNU_V3 | DMGL_TYPES | DMGL_ANSI | DMGL_PARAMS);
	
	if (demangled != NULL) {
		return demangled;
	} else {
		return mangled;
	}
}


const char *try_demangle_noprefix(const char *mangled)
{
	const char *demangled = try_demangle(mangled);
	
	if (strncmp(demangled, "typeinfo for ", 13) == 0) {
		return demangled + 13;
	} else if (strncmp(demangled, "vtable for ", 11) == 0) {
		return demangled + 11;
	} else {
		return demangled;
	}
}
