#include "all.h"


void symtab_elf_init(library_info_t *lib)
{
	lib->elf_hdr = lib->map;
	lib->elf = elf_memory(lib->map, lib->size);
	
	Elf_Scn *scn = NULL;
	while ((scn = elf_nextscn(lib->elf, scn)) != NULL) {
		GElf_Shdr shdr;
		gelf_getshdr(scn, &shdr);
		
		char *scn_name = elf_strptr(lib->elf, lib->elf_hdr->e_shstrndx,
			shdr.sh_name);
		
		if (shdr.sh_type == SHT_SYMTAB) {
			if (strcmp(scn_name, ".symtab") == 0) {
				lib->elf_symtab_shdr = shdr;
				lib->elf_symtab_data = elf_getdata(scn, NULL);
				lib->elf_symtab_count = shdr.sh_size / shdr.sh_entsize;
				//pr_debug("elf_symtab_count: %u\n", lib->elf_symtab_count);
			}
		} else if (shdr.sh_type == SHT_PROGBITS) {
			if (strcmp(scn_name, ".text") == 0) {
				//pr_debug("%s:%s %08x %08x\n", lib->name, scn_name,
				//	(uint32_t)shdr.sh_addr, (uint32_t)shdr.sh_size);
				lib->text_off  = shdr.sh_addr;
				lib->text_size = shdr.sh_size;
			} else if (strcmp(scn_name, ".data") == 0) {
				//pr_debug("%s:%s %08x %08x\n", lib->name, scn_name,
				//	(uint32_t)shdr.sh_addr, (uint32_t)shdr.sh_size);
				lib->data_off  = shdr.sh_addr;
				lib->data_size = shdr.sh_size;
			} else if (strcmp(scn_name, ".rodata") == 0) {
				//pr_debug("%s:%s %08x %08x\n", lib->name, scn_name,
				//	(uint32_t)shdr.sh_addr, (uint32_t)shdr.sh_size);
				lib->rodata_off  = shdr.sh_addr;
				lib->rodata_size = shdr.sh_size;
			} else if (strcmp(scn_name, ".data.rel.ro") == 0) {
				//pr_debug("%s:%s %08x %08x\n", lib->name, scn_name,
				//	(uint32_t)shdr.sh_addr, (uint32_t)shdr.sh_size);
				lib->datarelro_off  = shdr.sh_addr;
				lib->datarelro_size = shdr.sh_size;
			}
		} else if (shdr.sh_type == SHT_NOBITS) {
			if (strcmp(scn_name, ".bss") == 0) {
				//pr_debug("%s:%s %08x %08x\n", lib->name, scn_name,
				//	(uint32_t)shdr.sh_addr, (uint32_t)shdr.sh_size);
				lib->bss_off  = shdr.sh_addr;
				lib->bss_size = shdr.sh_size;
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
			.lib  = lib,
			.addr = sym.st_value,
			.size = sym.st_size,
			.name = elf_strptr(lib->elf,
				lib->elf_symtab_shdr.sh_link, sym.st_name),
			.name_demangled = try_demangle_noprefix(entry.name),
		};
		callback(&entry);
	}
}


bool symtab_elf_lookup_addr(library_info_t *lib, symbol_t *entry,
	uintptr_t addr)
{
	for (int i = 0; i < lib->elf_symtab_count; ++i) {
		GElf_Sym sym;
		gelf_getsym(lib->elf_symtab_data, i, &sym);
		
		//if (ELF32_ST_TYPE(sym.st_info) != type) {
		//	continue;
		//}
		
		if (sym.st_value == addr) {
			entry->lib  = lib;
			entry->addr = sym.st_value;
			entry->size = sym.st_size;
			entry->name = elf_strptr(lib->elf,
				lib->elf_symtab_shdr.sh_link, sym.st_name);
			entry->name_demangled = try_demangle_noprefix(entry->name);
			
			return true;
		}
		
		/*fprintf(stderr,
			"val %08x size %8x bind %2d type %2d name '%s'\n",
			val, size, bind, type,
			elf_strptr(lib->elf, shdr.sh_link, sym.st_name));*/
	}
	
	return false;
}
