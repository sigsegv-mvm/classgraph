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
	auto rtti =
		(const __class_type_info *)((uintptr_t)sym->lib->map + sym->addr);
	
	if (sym->lib->is_elf) {
		uintptr_t ti_vtable = *(const uintptr_t *)rtti;
		if (ti_vtable == vtable_for__class_type_info) {
			type_char = '-';
		} else if (ti_vtable == vtable_for__si_class_type_info) {
			type_char = '+';
		} else if (ti_vtable == vtable_for__vmi_class_type_info) {
			type_char = '#';
		} else {
			pr_warn("could not decipher typeinfo vtable ptr! (%08x)\n",
				ti_vtable);
		}
	}
	if (sym->lib->is_macho) {
		uintptr_t rtti_addr = (uintptr_t)rtti - (uintptr_t)sym->lib->map;
		int rtti_type = symtab_macho_check_rtti_reloc(sym->lib, rtti_addr);
		if (rtti_type == 1) {
			type_char = '-';
		} else if (rtti_type == 2) {
			type_char = '+';
		} else if (rtti_type == 3) {
			type_char = '#';
		} else {
			pr_warn("could not find reloc entry for rtti! (%08x)\n",
				rtti_addr);
		}
		
		/*
		symbol_t ti_base, ti_si, ti_vmi;
		
		assert(symtab_macho_lookup_name(sym->lib, &ti_base, "_ZTVN10__cxxabiv117__class_type_infoE"));
		assert(symtab_macho_lookup_name(sym->lib, &ti_si, "_ZTVN10__cxxabiv120__si_class_type_infoE"));
		assert(symtab_macho_lookup_name(sym->lib, &ti_vmi, "_ZTVN10__cxxabiv121__vmi_class_type_infoE"));
		
		uintptr_t rtti_addr = (uintptr_t)rtti - sym->lib->baseaddr;
		if (symtab_macho_check_reloc_for_addr(sym->lib, rtti_addr, &ti_base)) {
			//pr_debug("SUCCESS: base class\n");
			type_char = '-';
		} else if (symtab_macho_check_reloc_for_addr(sym->lib, rtti_addr, &ti_si)) {
			//pr_debug("SUCCESS: si class\n");
			type_char = '+';
		} else if (symtab_macho_check_reloc_for_addr(sym->lib, rtti_addr, &ti_vmi)) {
			//pr_debug("SUCCESS: vmi class\n");
			type_char = '#';
		} else {
			//pr_debug("FAILURE\n");
		}*/
		
		/*
		symbol_t rsym;
		if (symtab_macho_find_reloc_sym_for_addr(sym->lib, &rsym,
			rtti_addr)) {
			pr_debug("found relocation entry for %08x\n",
				rtti_addr);
		} else {
			pr_debug("could not find relocation entry for %08x\n",
				rtti_addr);
		}*/
	}
	
	printf("%s%s[%c] ", (level == 0 ? "\n" : ""),
		indent, type_char);
	printf("%s\n", sym->name_demangled);
	
	if (type_char == '+') {
		auto rtti_si = reinterpret_cast<const __si_class_type_info *>(rtti);
		
		symbol_t sym_base;
		if (symtab_addr_abs(&sym_base,
			(uintptr_t)(rtti_si->__base_type))) {
			recurse_typeinfo(level + 1, &sym_base);
		} else {
			pr_warn("could not find typeinfo symbol for base!\n");
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
			
			printf("%s %u: ",
				indent, i);
			printf("+%06x %s\n",
				offset, str_flags);
			
			symbol_t sym_base;
			if (symtab_addr_abs(&sym_base,
				(uintptr_t)(baseinfo->__base_type))) {
				recurse_typeinfo(level + 1, &sym_base);
			} else {
				pr_warn("could not find typeinfo symbol for base #%u!\n",
					i);
			}
		}
	}
}


std::vector<symbol_t> syms;


void sym_iterator(const symbol_t *sym)
{
	if (strncmp(sym->name, "_ZTI", 4) != 0 ||
		!r_match(sym->name_demangled)) {
		return;
	}
	
	syms.push_back(*sym);
}


bool sym_compare(const symbol_t& lhs, const symbol_t& rhs)
{
	int cmp = strcmp(lhs.name_demangled, rhs.name_demangled);
	return (cmp < 0);
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
	lib_init("libtier0_srv.so");
	lib_init("libvstdlib_srv.so");
	lib_init("soundemittersystem_srv.so");
	lib_init("vphysics_srv.so");
	lib_init("replay_srv.so");
	lib_init("dedicated_srv.so");
	lib_init("materialsystem_srv.so");
	lib_init("studiorender_srv.so");
	lib_init("shaderapiempty_srv.so");
	lib_init("datacache_srv.so");
	lib_init("scenefilecache_srv.so");
	
	//lib_init("engine.dylib");
	lib_init("server.dylib");
	//lib_init("client.dylib");
	//lib_init("filesystem_stdio.dylib");
	//lib_init("GameUI.dylib");
	//lib_init("inputsystem.dylib");
	//lib_init("launcher.dylib");
	//lib_init("libtier0.dylib");
	//lib_init("libvstdlib.dylib");
	//lib_init("materialsystem.dylib");
	//lib_init("replay.dylib");
	//lib_init("ServerBrowser.dylib");
	//lib_init("soundemittersystem.dylib");
	//lib_init("studiorender.dylib");
	//lib_init("vgui2.dylib");
	//lib_init("vguimatsurface.dylib");
	//lib_init("vphysics.dylib");
	//lib_init("datacache.dylib");
	//lib_init("scenefilecache.dylib");
	
	get_type_info_addrs();
	
	
	symtab_foreach(&sym_iterator);
	std::sort(syms.begin(), syms.end(), &sym_compare);
	for (const symbol_t& sym : syms) {
		recurse_typeinfo(0, &sym);
	}
	
	return 0;
}
