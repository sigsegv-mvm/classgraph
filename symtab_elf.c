#include "all.h"


static int symtab_elf_find_section(library_info_t *lib, Elf32_Word type, const char *name, GElf_Shdr *shdr_out, Elf_Scn **scn_out)
{
	int num_found = 0;
	
	Elf_Scn *scn = NULL;
	while ((scn = elf_nextscn(lib->elf, scn)) != NULL) {
		GElf_Shdr shdr;
		gelf_getshdr(scn, &shdr);
		
		if (shdr.sh_type != type) continue;
		
		if (name != NULL) {
			char *scn_name = elf_strptr(lib->elf, lib->elf_hdr->e_shstrndx, shdr.sh_name);
			if (strcmp(scn_name, name) != 0) continue;
		}
		
		if (shdr_out != NULL) memcpy(shdr_out, &shdr, sizeof(GElf_Shdr));
		if (scn_out  != NULL) *scn_out = scn;
		++num_found;
	}
	
	return num_found;
}


void symtab_elf_init(library_info_t *lib)
{
	lib->elf_hdr = lib->map;
	lib->elf = elf_memory(lib->map, lib->size);
	
	lib->depend_count = 0;
	
	GElf_Shdr symtab_shdr;
	Elf_Scn *symtab_scn;
	assert(symtab_elf_find_section(lib, SHT_SYMTAB, ".symtab", &symtab_shdr, &symtab_scn) == 1);
	lib->elf_symtab_shdr = symtab_shdr;
	lib->elf_symtab_data = elf_getdata(symtab_scn, NULL);
	lib->elf_symtab_count = symtab_shdr.sh_size / symtab_shdr.sh_entsize;
	
	Elf_Scn *dynstr_scn;
	assert(symtab_elf_find_section(lib, SHT_STRTAB, ".dynstr", NULL, &dynstr_scn) == 1);
	
	Elf_Scn *dynamic_scn;
	assert(symtab_elf_find_section(lib, SHT_DYNAMIC, ".dynamic", NULL, &dynamic_scn) == 1);
	Elf_Data *dynamic_data = elf_getdata(dynamic_scn, NULL);
	for (int i = 0; ; ++i) {
		GElf_Dyn dyn_entry;
		gelf_getdyn(dynamic_data, i, &dyn_entry);
		
		if (dyn_entry.d_tag == DT_NULL) break;
		
		if (dyn_entry.d_tag == DT_NEEDED) {
			lib->depends[lib->depend_count++] = strdup(elf_strptr(lib->elf, elf_ndxscn(dynstr_scn), dyn_entry.d_un.d_val));
		}
	}
}


void symtab_elf_foreach(library_info_t *lib, void (*callback)(const symbol_t *))
{
	for (int i = 0; i < lib->elf_symtab_count; ++i) {
		GElf_Sym sym;
		gelf_getsym(lib->elf_symtab_data, i, &sym);
		
		symbol_t entry = {
			.lib            = lib,
			.addr           = sym.st_value,
			.size           = sym.st_size,
			.name           = elf_strptr(lib->elf, lib->elf_symtab_shdr.sh_link, sym.st_name),
			.name_demangled = try_demangle_noprefix(entry.name),
		};
		
		callback(&entry);
	}
}
