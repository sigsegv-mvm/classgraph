#include "all.h"
#include <cxxabi.h>
#include "regex.h"


using __cxxabiv1::__base_class_type_info;
using __cxxabiv1::__class_type_info;
using __cxxabiv1::__si_class_type_info;
using __cxxabiv1::__vmi_class_type_info;


static uintptr_t vtable_for__class_type_info;
static uintptr_t vtable_for__si_class_type_info;
static uintptr_t vtable_for__vmi_class_type_info;


std::set<symbol_t, bool (*)(const symbol_t&, const symbol_t&)> syms([](const symbol_t& lhs, const symbol_t& rhs){ return (strcmp(lhs.name_demangled, rhs.name_demangled) < 0); });
std::map<uintptr_t, symbol_t> symcache;


extern "C" bool symtab_lookup_addr(library_info_t *lib, symbol_t *entry, uintptr_t addr)
{
	for (const auto& pair : symcache) {
		const uintptr_t& addr_i = pair.first;
		const symbol_t& sym_i   = pair.second;
		
		if (addr == addr_i && lib == sym_i.lib) {
			*entry = sym_i;
			return true;
		}
	}
	
	return false;
}


void get_type_info_addrs(void)
{
	__class_type_info     ti_base(NULL);
	__si_class_type_info  ti_si  (NULL, NULL);
	__vmi_class_type_info ti_vmi (NULL, 0);
	
	vtable_for__class_type_info     = *((uintptr_t *)&ti_base);
	vtable_for__si_class_type_info  = *((uintptr_t *)&ti_si);
	vtable_for__vmi_class_type_info = *((uintptr_t *)&ti_vmi);
	
	assert(vtable_for__class_type_info     != 0);
	assert(vtable_for__si_class_type_info  != 0);
	assert(vtable_for__vmi_class_type_info != 0);
}


void recurse_typeinfo(int level, const symbol_t *sym)
{
	if (sym->addr == 0) {
		return;
	}
	
	char indent[1024];
	char *p = indent;
	for (int i = 0; i < level; ++i) {
		*(p++) = ' ';
		*(p++) = '|';
		*(p++) = ' ';
		*(p++) = ' ';
	}
	*p = '\0';
	
	char type_char = '?';
	const __class_type_info *rtti = nullptr;
	
	if (sym->lib->is_elf) {
		rtti = (const __class_type_info *)(sym->lib->baseaddr + sym->addr);
		
		uintptr_t ti_vtable = *(const uintptr_t *)rtti;
		if (ti_vtable == vtable_for__class_type_info) {
			type_char = '-';
		} else if (ti_vtable == vtable_for__si_class_type_info) {
			type_char = '+';
		} else if (ti_vtable == vtable_for__vmi_class_type_info) {
			type_char = '#';
		} else {
			pr_warn("could not decipher typeinfo vtable ptr! (%08x)\n", ti_vtable);
		}
	}
	if (sym->lib->is_macho) {
		rtti = (const __class_type_info *)((uintptr_t)sym->lib->map + sym->addr);
		
		symbol_t rsym;
		if (symtab_macho_find_reloc_sym_for_addr(sym->lib, &rsym, sym->addr)) {
			if (strcmp(rsym.name, "_ZTVN10__cxxabiv117__class_type_infoE") == 0) {
				type_char = '-';
			} else if (strcmp(rsym.name, "_ZTVN10__cxxabiv120__si_class_type_infoE") == 0) {
				type_char = '+';
			} else if (strcmp(rsym.name, "_ZTVN10__cxxabiv121__vmi_class_type_infoE") == 0) {
				type_char = '#';
			} else {
				pr_warn("whoa, got unexpected reloc symbol:\n  %08x '%s'\n", rsym.addr, rsym.name_demangled);
			}
		} else {
			pr_warn("could not find reloc entry for rtti! (%08x)\n", sym->addr);
		}
	}
	
	printf("%s%s[%c] ", (level == 0 ? "\n" : ""), indent, type_char);
	printf("%s\n", sym->name_demangled);
	
	if (type_char == '+') {
		auto rtti_si = reinterpret_cast<const __si_class_type_info *>(rtti);
		
		symbol_t sym_base;
		if (symtab_addr_abs(&sym_base, (uintptr_t)(rtti_si->__base_type))) {
			recurse_typeinfo(level + 1, &sym_base);
		} else {
			pr_warn("could not find typeinfo symbol for base!\n");
		}
	} else if (type_char == '#') {
		auto rtti_vmi = reinterpret_cast<const __vmi_class_type_info *>(rtti);
		
		for (unsigned int i = 0; i != rtti_vmi->__base_count; ++i) {
			auto *baseinfo = rtti_vmi->__base_info + i;
			
			ptrdiff_t offset = baseinfo->__offset();
			unsigned long flags = (unsigned long)baseinfo->__offset_flags & ~(ULONG_MAX << __base_class_type_info::__offset_shift);
			
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
			
			printf("%s %u: ", indent, i);
			printf("+%06x %s\n", offset, str_flags);
			
			symbol_t sym_base;
			
			auto cache_entry = symcache.find((uintptr_t)(baseinfo->__base_type));
			if (cache_entry != symcache.end()) {
				sym_base = (*cache_entry).second;
				recurse_typeinfo(level + 1, &sym_base);
			} else if (symtab_addr_abs(&sym_base, (uintptr_t)(baseinfo->__base_type))) {
				recurse_typeinfo(level + 1, &sym_base);
			} else {
				pr_warn("could not find typeinfo symbol for base #%u!\n", i);
			}
		}
	}
}


int main(int argc, char **argv)
{
	pr_info("classgraph\n\n");
	
	if (argc != 3) {
		errx(1, "usage: %s <library> <filter>\n", argv[0]);
	}
	
	lib_init(argv[1]);
	r_init(argv[2]);
	
	get_type_info_addrs();
	
	symtab_foreach([](const symbol_t *sym){
		if (strncmp(sym->name, "_ZTI", 4) != 0 || !r_match(sym->name_demangled)) return;
		syms.insert(*sym);
		symcache[sym->addr] = *sym;
	});
	
	for (const symbol_t& sym : syms) {
		recurse_typeinfo(0, &sym);
	}
	
	return 0;
}
