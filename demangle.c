#include "all.h"


/* transform all cases of "> >" into "">>" */
static const char *fixup_templates(const char *str_in)
{
	if (strstr(str_in, "> >") == NULL) return str_in;
	
	size_t len_in = strlen(str_in);
	char *str_out = malloc(len_in);
	
	char *p_out = str_out;
	for (size_t i = 0; i <= len_in; ++i) {
		if (i >= 2 && str_in[i] == '>' && str_in[i - 1] == ' ' && str_in[i - 2] == '>') {
			--p_out;
		}
		
		*(p_out++) = str_in[i];
	}
	
	free((void *)str_in);
	
	return str_out;
}


const char *try_demangle(const char *mangled)
{
	const char *demangled = cplus_demangle(mangled, DMGL_GNU_V3 | DMGL_TYPES | DMGL_ANSI | DMGL_PARAMS);
	
	if (demangled != NULL) {
		return fixup_templates(demangled);
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
