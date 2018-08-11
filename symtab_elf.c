#include "all.h"


void symtab_elf_init(library_info_t *lib)
{
	lib->elf_hdr = lib->map;
	lib->elf = elf_memory(lib->map, lib->size);
	
	Elf_Scn *scn = NULL;
	while ((scn = elf_nextscn(lib->elf, scn)) != NULL) {
		GElf_Shdr shdr;
		gelf_getshdr(scn, &shdr);
		
		char *scn_name = elf_strptr(lib->elf, lib->elf_hdr->e_shstrndx, shdr.sh_name);
		
		if (shdr.sh_type == SHT_SYMTAB) {
			if (strcmp(scn_name, ".symtab") == 0) {
				lib->elf_symtab_shdr = shdr;
				lib->elf_symtab_data = elf_getdata(scn, NULL);
				lib->elf_symtab_count = shdr.sh_size / shdr.sh_entsize;
				//pr_debug("elf_symtab_count: %u\n", lib->elf_symtab_count);
			}
		}
		
		/*pr_debug("type %08x flags %c%c%c%c name '%s'\n",
			shdr.sh_type,
			(shdr.sh_flags & SHF_WRITE     ? 'W' : ' '),
			(shdr.sh_flags & SHF_ALLOC     ? 'A' : ' '),
			(shdr.sh_flags & SHF_EXECINSTR ? 'X' : ' '),
			(shdr.sh_flags & SHF_STRINGS   ? 'S' : ' '),
			elf_strptr(lib->elf, lib->elf_hdr->e_shstrndx, shdr.sh_name));*/
	}
	
	
	//munmap(lib->map, lib->size);
	//close(lib->fd);
}


void symtab_elf_foreach(library_info_t *lib, void (*callback)(const symbol_t *))
{
	for (int i = 0; i < lib->elf_symtab_count; ++i) {
		GElf_Sym sym;
		gelf_getsym(lib->elf_symtab_data, i, &sym);
		//if (ELF32_ST_TYPE(sym.st_info) != type) {
		//	continue;
		//}
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
