#include "all.h"
#include <cxxabi.h>


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
	const abi::__class_type_info *rtti = nullptr;
	
	if (sym->lib->is_elf) {
		rtti = (const abi::__class_type_info *)(sym->lib->baseaddr + sym->addr);
		
		try
		{
			if (typeid(*rtti) == typeid(abi::__class_type_info)) {
				type_char = '-';
			} else if (typeid(*rtti) == typeid(abi::__si_class_type_info)) {
				type_char = '+';
			} else if (typeid(*rtti) == typeid(abi::__vmi_class_type_info)) {
				type_char = '#';
			} else if (typeid(*rtti) == typeid(abi::__array_type_info)) {
				type_char = 'A';
			} else if (typeid(*rtti) == typeid(abi::__enum_type_info)) {
				type_char = 'E';
			} else if (typeid(*rtti) == typeid(abi::__function_type_info)) {
				type_char = 'F';
			} else if (typeid(*rtti) == typeid(abi::__pointer_to_member_type_info)) {
				type_char = 'M';
			} else if (typeid(*rtti) == typeid(abi::__pointer_type_info)) {
				type_char = '*';
			} else if (typeid(*rtti) == typeid(abi::__fundamental_type_info)) {
				type_char = '.';
			} else if (strcmp(typeid(*rtti).name(), "St19__iosfail_type_info") == 0) {
				/* just ignore this garbage */
			} else {
				pr_warn("could not decipher typeinfo! (%08x)\n", sym->addr);
				pr_warn("typeid(*rtti).name() = \"%s\"\n", typeid(*rtti).name());
			}
		} catch (const std::bad_typeid& e) {
			pr_warn("could not decipher typeinfo! (typeid failure: \"%s\") (%08x)\n", e.what(), sym->addr);
		}
	}
	if (sym->lib->is_macho) {
		rtti = (const abi::__class_type_info *)symtab_macho_get_ptr(sym->lib, sym->addr, sizeof(abi::__class_type_info));
		
		symbol_t rsym;
		if (symtab_macho_find_reloc_sym_for_addr(sym->lib, &rsym, sym->addr)) {
			static std::regex re_typeinfo(R"REGEX(^_ZTVN10__cxxabiv1[[:digit:]]{2}__(class|si_class|vmi_class|array|enum|function|pointer_to_member|pointer|fundamental)_type_infoE.*$)REGEX",
				std::regex::optimize | std::regex::egrep);

			std::cmatch m_typeinfo;
			if (std::regex_match(rsym.name, m_typeinfo, re_typeinfo, std::regex_constants::match_default) && m_typeinfo.size() == 2) {
				if (m_typeinfo[1] == "class") {
					type_char = '-';
				} else if (m_typeinfo[1] == "si_class") {
					type_char = '+';
				} else if (m_typeinfo[1] == "vmi_class") {
					type_char = '#';
				} else if (m_typeinfo[1] == "array") {
					type_char = 'A';
				} else if (m_typeinfo[1] == "enum") {
					type_char = 'E';
				} else if (m_typeinfo[1] == "function") {
					type_char = 'F';
				} else if (m_typeinfo[1] == "pointer_to_member") {
					type_char = 'M';
				} else if (m_typeinfo[1] == "pointer") {
					type_char = '*';
				} else if (m_typeinfo[1] == "fundamental") {
					type_char = '.';
				} else {
					assert(false);
				}
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
		auto rtti_si = reinterpret_cast<const abi::__si_class_type_info *>(rtti);
		
		symbol_t sym_base;
		if (symtab_addr_abs(&sym_base, (uintptr_t)(rtti_si->__base_type))) {
			recurse_typeinfo(level + 1, &sym_base);
		} else {
			pr_warn("could not find typeinfo symbol for base!\n");
		}
	} else if (type_char == '#') {
		auto rtti_vmi = reinterpret_cast<const abi::__vmi_class_type_info *>(rtti);
		
		for (unsigned int i = 0; i != rtti_vmi->__base_count; ++i) {
			auto *baseinfo = rtti_vmi->__base_info + i;
			
			ptrdiff_t offset = baseinfo->__offset();
			unsigned long flags = (unsigned long)baseinfo->__offset_flags & ~(ULONG_MAX << abi::__base_class_type_info::__offset_shift);
			
			std::vector<std::string> flag_strs;
			if ((flags & abi::__base_class_type_info::__virtual_mask  ) != 0) flag_strs.emplace_back("virtual");
			if ((flags & abi::__base_class_type_info::__public_mask   ) != 0) flag_strs.emplace_back("public");
			if ((flags & (1 << abi::__base_class_type_info::__hwm_bit)) != 0) flag_strs.emplace_back("HWM");
			std::string str_flags = boost::algorithm::join(flag_strs, " ");
			
			printf("%s %u: ", indent, i);
			if (offset >= 0) {
				printf("+0x%04X %s\n", offset, str_flags.c_str());
			} else {
				printf("-0x%04X %s\n", -offset, str_flags.c_str());
			}
			
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


#warning TODO: regression-test on Mach-O binaries!
int main(int argc, char **argv)
{
	pr_info("classgraph\n\n");
	
	if (argc < 2 || argc > 3) {
		errx(1, "usage: %s <library> [filter]\n", argv[0]);
	}
	
	static const char *arg_lib    = (argc > 1 ? argv[1] : nullptr);
	static const char *arg_filter = (argc > 2 ? argv[2] : nullptr);
	
	lib_init(true, arg_lib);
	
	/* only apply regex filter if we were actually given a third arg */
	static auto l_filter = (argc < 3
		? [](const char *sym_name){ return true; }
		: [](const char *sym_name){
			static std::regex r(arg_filter, std::regex::icase | std::regex::nosubs | std::regex::optimize | std::regex::egrep);
			return std::regex_match(sym_name, r, std::regex_constants::match_any);
		});
	
	symtab_foreach([](const symbol_t *sym){
		if (strncmp(sym->name, "_ZTI", 4) != 0) return;
		if (!l_filter(sym->name))               return;
		syms.insert(*sym);
		symcache[sym->addr] = *sym;
	});
	
	for (const symbol_t& sym : syms) {
		recurse_typeinfo(0, &sym);
	}
	
	return 0;
}
