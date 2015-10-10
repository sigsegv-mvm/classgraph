#include "all.h"
#include <cxxabi.h>
#include "regex.h"


using __cxxabiv1::__base_class_type_info;
using __cxxabiv1::__class_type_info;
using __cxxabiv1::__si_class_type_info;
using __cxxabiv1::__vmi_class_type_info;


const char *try_demangle(const char *mangled)
{
	const char *demangled = cplus_demangle(mangled,
		DMGL_GNU_V3 | DMGL_TYPES | DMGL_ANSI | DMGL_PARAMS);
	
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


static uintptr_t vtable_for__class_type_info;
static uintptr_t vtable_for__si_class_type_info;
static uintptr_t vtable_for__vmi_class_type_info;


void get_type_info_addrs(void)
{
	__class_type_info     ti_base(NULL);
	__si_class_type_info  ti_si(NULL, NULL);
	__vmi_class_type_info ti_vmi(NULL, 0);
	
	vtable_for__class_type_info     = *((uintptr_t *)&ti_base);
	vtable_for__si_class_type_info  = *((uintptr_t *)&ti_si);
	vtable_for__vmi_class_type_info = *((uintptr_t *)&ti_vmi);
	
	assert(vtable_for__class_type_info     != 0);
	assert(vtable_for__si_class_type_info  != 0);
	assert(vtable_for__vmi_class_type_info != 0);
}


void recurse_typeinfo(int level, const symbol_t *sym)
{
	char indent[1024];
	char *p = indent;
	for (int i = 0; i < level; ++i) {
		*(p++) = ' ';
		*(p++) = '|';
		*(p++) = ' ';
		*(p++) = ' ';
	}
	*p = '\0';
	
	
	char type_char;
	auto rtti =
		(const __class_type_info *)(sym->lib->baseaddr + sym->addr);
	
	uintptr_t ti_vtable = *(const uintptr_t *)rtti;
	if (ti_vtable == vtable_for__class_type_info) {
		type_char = '-';
	} else if (ti_vtable == vtable_for__si_class_type_info) {
		type_char = '+';
	} else if (ti_vtable == vtable_for__vmi_class_type_info) {
		type_char = '#';
	} else {
		pr_warn("\n%scould not decipher typeinfo vtable ptr!",
			indent);
		type_char = '?';
	}
	
	const char *demangled = try_demangle_noprefix(sym->name);
	pr_info("%s%s[%c] ", (level == 0 ? "\n" : ""),
		indent, type_char);
	pr_debug("%s\n", demangled);
	
	if (type_char == '+') {
		auto rtti_si = reinterpret_cast<const __si_class_type_info *>(rtti);
		
		symbol_t sym_base;
		if (symtab_obj_addr_abs(&sym_base,
			(uintptr_t)(rtti_si->__base_type))) {
			recurse_typeinfo(level + 1, &sym_base);
		} else {
			pr_warn("%s    could not find typeinfo symbol for base!\n",
				indent);
		}
	} else if (type_char == '#') {
		auto rtti_vmi = reinterpret_cast<const __vmi_class_type_info *>(rtti);
		
		for (unsigned int i = 0; i != rtti_vmi->__base_count; ++i) {
			auto *baseinfo = rtti_vmi->__base_info + i;
			
			ptrdiff_t offset = baseinfo->__offset();
			unsigned long flags = (unsigned long)baseinfo->__offset_flags &
				~(ULONG_MAX << __base_class_type_info::__offset_shift);
			
			char str_flags[1024];
			str_flags[0] = '\0';
			if ((flags & __base_class_type_info::__virtual_mask) != 0) {
				if (str_flags[0] != '\0') {
					strlcat(str_flags, " ", sizeof(str_flags));
				}
				strlcat(str_flags, "virtual", sizeof(str_flags));
			}
			if ((flags & __base_class_type_info::__public_mask) != 0) {
				if (str_flags[0] != '\0') {
					strlcat(str_flags, " ", sizeof(str_flags));
				}
				strlcat(str_flags, "public", sizeof(str_flags));
			}
			if ((flags & (1 << __base_class_type_info::__hwm_bit)) != 0) {
				if (str_flags[0] != '\0') {
					strlcat(str_flags, " ", sizeof(str_flags));
				}
				strlcat(str_flags, "HWM", sizeof(str_flags));
			}
			
			pr_info("%s %u: ",
				indent, i);
			pr_debug("+%06x %s\n",
				offset, str_flags);
			
			symbol_t sym_base;
			if (symtab_obj_addr_abs(&sym_base,
				(uintptr_t)(baseinfo->__base_type))) {
				recurse_typeinfo(level + 1, &sym_base);
			} else {
				pr_warn("%scould not find typeinfo symbol for base #%u!\n",
					indent, i);
			}
		}
	}
}


void sym_iterator(const symbol_t *sym)
{
	if (strncmp(sym->name, "_ZTI", 4) != 0) {
		return;
	}
	
	if (!r_match(try_demangle_noprefix(sym->name))) {
		return;
	}
	
	recurse_typeinfo(0, sym);
}


int main(int argc, char **argv)
{
	pr_info("classgraph\n\n");
	
	if (argc != 2) {
		errx(1, "usage: %s <filter>\n", argv[0]);
	}
	r_init(argv[1]);
	
	lib_init("engine_srv.so");
	lib_init("server_srv.so");
	
	get_type_info_addrs();
	
	symtab_foreach(&sym_iterator, STT_OBJECT);
	
	return 0;
}
