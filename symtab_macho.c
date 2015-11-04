#include "all.h"


static struct nlist *macho_get_sym_by_index(library_info_t *lib, int index)
{
	uintptr_t sym_off = (uintptr_t)lib->map + lib->macho_symtab_off;
	return (struct nlist *)sym_off + index;
}

static const char *macho_get_sym_name(library_info_t *lib, struct nlist *sym)
{
	if (sym->n_un.n_strx == 0) {
		return "";
	} else {
		const char *name = (const char *)((uintptr_t)lib->map +
			lib->macho_strtab_off + sym->n_un.n_strx);
		
		if (name[0] == '_') {
			return name + 1;
		} else {
			return name;
		}
	}
}

static uintptr_t macho_get_sym_addr(library_info_t *lib, struct nlist *sym)
{
	if ((sym->n_type & N_STAB) != 0) {
		return 0;
	}
	
	switch (sym->n_type & N_TYPE) {
	case N_UNDF:
	case N_PBUD:
		return 0;
	case N_ABS:
	case N_SECT:
		return sym->n_value;
	case N_INDR:
		assert(false);
	default:
		assert(false);
	}
}


void symtab_macho_init(library_info_t *lib)
{
	lib->macho_hdr = lib->map;
	
	//pr_debug("MACH-O INIT\n");
	
	assert(lib->macho_hdr->magic      == MH_MAGIC);
	assert(lib->macho_hdr->cputype    == CPU_TYPE_X86);
	assert(lib->macho_hdr->cpusubtype == CPU_SUBTYPE_X86_ALL);
	assert(lib->macho_hdr->filetype   == MH_DYLIB);
	
	//pr_debug("ncmds:      %08x\n", lib->macho_hdr->ncmds);
	//pr_debug("sizeofcmds: %08x\n", lib->macho_hdr->sizeofcmds);
	//pr_debug("flags:      %08x\n", lib->macho_hdr->flags);
	
	
	lib->macho_sect_count = 0;
	
	uintptr_t cmd_addr = (uintptr_t)lib->map + sizeof(struct mach_header);
	for (int i = 0; i < lib->macho_hdr->ncmds; ++i) {
		struct load_command *cmd = (struct load_command *)cmd_addr;
		
		//const char *cmd_name = "???";
		//if (cmd->cmd == LC_SEGMENT) {
		//	cmd_name = "LC_SEGMENT";
		//} else if (cmd->cmd == LC_SYMTAB) {
		//	cmd_name = "LC_SYMTAB";
		//}
		
		//pr_debug("load_command %d:\n", i);
		//pr_debug("  cmd:       %08x %s\n", cmd->cmd, cmd_name);
		//pr_debug("  cmdsize:   %08x\n", cmd->cmdsize);
		
		if (cmd->cmd == LC_SEGMENT) {
			struct segment_command *seg_cmd = (struct segment_command *)cmd;
			
			//pr_debug("  segname:   '%s'\n", seg_cmd->segname);
			//pr_debug("  vmaddr:    %08x\n", seg_cmd->vmaddr);
			//pr_debug("  vmsize:    %08x\n", seg_cmd->vmsize);
			//pr_debug("  fileoff:   %08x\n", seg_cmd->fileoff);
			//pr_debug("  filesize:  %08x\n", seg_cmd->filesize);
			//pr_debug("  maxprot:   %08x\n", seg_cmd->maxprot);
			//pr_debug("  initprot:  %08x\n", seg_cmd->initprot);
			//pr_debug("  nsects:    %08x\n", seg_cmd->nsects);
			//pr_debug("  flags:     %08x\n", seg_cmd->flags);
			
			uintptr_t sect_addr = cmd_addr + sizeof(struct segment_command);
			for (int j = 0; j < seg_cmd->nsects; ++j) {
				struct section *sect = (struct section *)sect_addr;
				
				pr_debug("  section %d:\n", j);
				pr_debug("    sectname:  '%s'\n", sect->sectname);
				pr_debug("    segname:   '%s'\n", sect->segname);
				pr_debug("    addr:      %08x\n", sect->addr);
				pr_debug("    size:      %08x\n", sect->size);
				pr_debug("    offset:    %08x\n", sect->offset);
				pr_debug("    align:     %08x\n", sect->align);
				pr_debug("    reloff:    %08x\n", sect->reloff);
				pr_debug("    nreloc:    %08x\n", sect->nreloc);
				pr_debug("    flags:     %08x\n", sect->flags);
				pr_debug("    reserved1: %08x\n", sect->reserved1);
				pr_debug("    reserved2: %08x\n", sect->reserved2);
				
				assert(lib->macho_sect_count <= 255);
				lib->macho_sects[lib->macho_sect_count++] = sect;
				
				if (strcmp(sect->segname, "__DATA") == 0) {
					if (strcmp(sect->sectname, "__data") == 0) {
						lib->data_off  = sect->offset;
						lib->data_size = sect->size;
					} else if (strcmp(sect->sectname, "__const") == 0) {
						lib->rodata_off  = sect->offset;
						lib->rodata_size = sect->size;
					}
				}
				// TODO: text, datarelro, bss
				
				sect_addr += sizeof(struct section);
			}
		} else if (cmd->cmd == LC_SYMTAB) {
			struct symtab_command *symtab_cmd = (struct symtab_command *)cmd;
			
			//pr_debug("  symoff:    %08x\n", symtab_cmd->symoff);
			//pr_debug("  nsyms:     %08x\n", symtab_cmd->nsyms);
			//pr_debug("  stroff:    %08x\n", symtab_cmd->stroff);
			//pr_debug("  strsize:   %08x\n", symtab_cmd->strsize);
			
			lib->macho_symtab_off   = symtab_cmd->symoff;
			lib->macho_symtab_count = symtab_cmd->nsyms;
			lib->macho_strtab_off   = symtab_cmd->stroff;
			lib->macho_strtab_size  = symtab_cmd->strsize;
		} else if (cmd->cmd == LC_DYSYMTAB) {
			struct dysymtab_command *dysymtab_cmd =
				(struct dysymtab_command *)cmd;
			
			pr_debug("  ilocalsym:      %08x\n", dysymtab_cmd->ilocalsym);
			pr_debug("  nlocalsym:      %08x\n", dysymtab_cmd->nlocalsym);
			pr_debug("  iextdefsym:     %08x\n", dysymtab_cmd->iextdefsym);
			pr_debug("  nextdefsym:     %08x\n", dysymtab_cmd->nextdefsym);
			pr_debug("  iundefsym:      %08x\n", dysymtab_cmd->iundefsym);
			pr_debug("  nundefsym:      %08x\n", dysymtab_cmd->nundefsym);
			pr_debug("  tocoff:         %08x\n", dysymtab_cmd->tocoff);
			pr_debug("  ntoc:           %08x\n", dysymtab_cmd->ntoc);
			pr_debug("  modtaboff:      %08x\n", dysymtab_cmd->modtaboff);
			pr_debug("  nmodtab:        %08x\n", dysymtab_cmd->nmodtab);
			pr_debug("  extrefsymoff:   %08x\n", dysymtab_cmd->extrefsymoff);
			pr_debug("  nextrefsyms:    %08x\n", dysymtab_cmd->nextrefsyms);
			pr_debug("  indirectsymoff: %08x\n", dysymtab_cmd->indirectsymoff);
			pr_debug("  nindirectsyms:  %08x\n", dysymtab_cmd->nindirectsyms);
			pr_debug("  extreloff:      %08x\n", dysymtab_cmd->extreloff);
			pr_debug("  nextrel:        %08x\n", dysymtab_cmd->nextrel);
			pr_debug("  locreloff:      %08x\n", dysymtab_cmd->locreloff);
			pr_debug("  nlocrel:        %08x\n", dysymtab_cmd->nlocrel);
			
			lib->macho_ext_reloc_off   = dysymtab_cmd->extreloff;
			lib->macho_ext_reloc_count = dysymtab_cmd->nextrel;
		}
		
		cmd_addr += cmd->cmdsize;
	}
	
	
	lib->macho_symidx_base = -1;
	lib->macho_symidx_si   = -1;
	lib->macho_symidx_vmi  = -1;
	
	uintptr_t sym_off = (uintptr_t)lib->map + lib->macho_symtab_off;
	for (int i = 0; i < lib->macho_symtab_count; ++i) {
		struct nlist *sym = (struct nlist *)sym_off + i;
		
		if (strcmp(macho_get_sym_name(lib, sym),
			"_ZTVN10__cxxabiv117__class_type_infoE") == 0) {
			lib->macho_symidx_base = i;
		} else if (strcmp(macho_get_sym_name(lib, sym),
			"_ZTVN10__cxxabiv120__si_class_type_infoE") == 0) {
			lib->macho_symidx_si = i;
		} else if (strcmp(macho_get_sym_name(lib, sym),
			"_ZTVN10__cxxabiv121__vmi_class_type_infoE") == 0) {
			lib->macho_symidx_vmi = i;
		}
	}
}


void symtab_macho_foreach(library_info_t *lib,
	void (*callback)(const symbol_t *))
{
	uintptr_t sym_off = (uintptr_t)lib->map + lib->macho_symtab_off;
	for (int i = 0; i < lib->macho_symtab_count; ++i) {
		struct nlist *sym = (struct nlist *)sym_off + i;
		
		//pr_debug("SYMBOL %d\n", i);
		//pr_debug("  n_strx:  %08x\n", sym->n_un.n_strx);
		//pr_debug("  n_type:  %02x\n", sym->n_type);
		//pr_debug("  n_sect:  %02x\n", sym->n_sect);
		//pr_debug("  n_desc:  %04x\n", sym->n_desc);
		//pr_debug("  n_value: %08x\n", sym->n_value);
		
		//if (strstr(macho_get_sym_name(lib, sym), "class_type_info") != NULL) {
		//	pr_debug("lib '%s' sym #%d\n", lib->name, i);
		//	pr_debug("  name '%s'\n", macho_get_sym_name(lib, sym));
		//	pr_debug("  addr %08x\n", macho_get_sym_addr(lib, sym));
		//	pr_debug("  val  %08x\n", sym->n_value);
		//	pr_debug("  sect %02x\n", sym->n_sect);
		//	pr_debug("  type %02x\n", sym->n_type);
		//}
		
		if ((sym->n_type & N_EXT) != 0 || (sym->n_type & N_STAB) != 0) {
			/* skip external and debug entries */
			continue;
		}
		
		symbol_t entry = {
			.lib  = lib,
			.addr = macho_get_sym_addr(lib, sym),
			.size = 0,
			.name = macho_get_sym_name(lib, sym),
			.name_demangled = try_demangle_noprefix(entry.name),
		};
		callback(&entry);
		
		/*if (strstr(entry.name, "IFileSystem") != NULL) {
			pr_debug("lib '%s' sym #%d\n", lib->name, i);
			pr_debug("  name '%s'\n", entry.name);
			pr_debug("  addr %08x\n", entry.addr);
			pr_debug("  val  %08x\n", sym->n_value);
			pr_debug("  sect %02x\n", sym->n_sect);
			pr_debug("  type %02x\n", sym->n_type);
		}*/
		
		//pr_debug("%08x: type %02x value %08x addr %08x name '%s'\n",
		//	i, sym->n_type, sym->n_value, entry.addr, entry.name);
	}
}


bool symtab_macho_lookup_name(library_info_t *lib, symbol_t *entry,
	const char *name)
{
	uintptr_t sym_off = (uintptr_t)lib->map + lib->macho_symtab_off;
	for (int i = 0; i < lib->macho_symtab_count; ++i) {
		struct nlist *sym = (struct nlist *)sym_off + i;
		
		#warning should this be commented out?
		//if ((sym->n_type & N_EXT) != 0 || (sym->n_type & N_STAB) != 0) {
		//	/* skip external and debug entries */
		//	continue;
		//}
		
		if (strcmp(name, macho_get_sym_name(lib, sym)) == 0) {
			entry->lib  = lib;
			entry->addr = macho_get_sym_addr(lib, sym);
			entry->size = 0;
			entry->name = name;
			entry->name_demangled = try_demangle_noprefix(entry->name);
			
			return true;
		}
	}
	
	return false;
}

bool symtab_macho_lookup_addr(library_info_t *lib, symbol_t *entry,
	uintptr_t addr)
{
	uintptr_t sym_off = (uintptr_t)lib->map + lib->macho_symtab_off;
	for (int i = 0; i < lib->macho_symtab_count; ++i) {
		struct nlist *sym = (struct nlist *)sym_off + i;
		
		if ((sym->n_type & N_EXT) != 0 || (sym->n_type & N_STAB) != 0) {
			/* skip external and debug entries */
			continue;
		}
		
		if (addr == macho_get_sym_addr(lib, sym)) {
			entry->lib  = lib;
			entry->addr = addr;
			entry->size = 0;
			entry->name = macho_get_sym_name(lib, sym);
			entry->name_demangled = try_demangle_noprefix(entry->name);
			
			return true;
		}
	}
	
	return false;
}


/* given a particular address, find out if there is a relocation entry for that
 * address, and if so, return some information about the symbol being relocated
 * to the address */
bool symtab_macho_find_reloc_sym_for_addr(library_info_t *lib, symbol_t *entry,
	uintptr_t addr)
{
	pr_debug("%s: %s\n", __func__, lib->name, lib->macho_sect_count);
	
	for (int i = 0; i < lib->macho_sect_count; ++i) {
		struct section *sect = lib->macho_sects[i];
		pr_debug("%s %d %08x\n", lib->name, i, sect);
		pr_debug("  %08x %08x\n", sect->reloff, sect->nreloc);
		
		uintptr_t reloc_addr = (uintptr_t)lib->map + sect->reloff;
		for (int j = 0; j < sect->nreloc; ++j) {
			struct relocation_info *reloc =
				(struct relocation_info *)reloc_addr + j;
			
			pr_debug("lib %s sect %d reloc %d\n", lib->name, i, j);
			pr_debug("  r_address:   %08x\n", reloc->r_address);
			pr_debug("  r_symbolnum: %06x\n", reloc->r_symbolnum);
			pr_debug("  r_pcrel:     %01x\n", reloc->r_pcrel);
			pr_debug("  r_length:    %01x\n", reloc->r_length);
			pr_debug("  r_extern:    %01x\n", reloc->r_extern);
			pr_debug("  r_type:      %01x\n", reloc->r_type);
			
			if (reloc->r_address == addr && !reloc->r_extern) {
				struct nlist *sym =
					macho_get_sym_by_index(lib, reloc->r_symbolnum);
				
				entry->lib  = lib;
				entry->addr = macho_get_sym_addr(lib, sym);
				entry->size = 0;
				entry->name = macho_get_sym_name(lib, sym);
				entry->name_demangled = try_demangle_noprefix(entry->name);
				
				return true;
			}
		}
	}
	
	
	return false;
}

int symtab_macho_check_rtti_reloc(library_info_t *lib, uintptr_t addr)
{
	struct relocation_info reloc_base = {
		.r_address   = addr,
		.r_symbolnum = lib->macho_symidx_base,
		.r_pcrel     = 0b0,
		.r_length    = 0b10,
		.r_extern    = 0b1,
		.r_type      = 0b0000,
	};
	struct relocation_info reloc_si = {
		.r_address   = addr,
		.r_symbolnum = lib->macho_symidx_si,
		.r_pcrel     = 0b0,
		.r_length    = 0b10,
		.r_extern    = 0b1,
		.r_type      = 0b0000,
	};
	struct relocation_info reloc_vmi = {
		.r_address   = addr,
		.r_symbolnum = lib->macho_symidx_vmi,
		.r_pcrel     = 0b0,
		.r_length    = 0b10,
		.r_extern    = 0b1,
		.r_type      = 0b0000,
	};
	
	for (int i = 0; i < lib->macho_ext_reloc_count; ++i) {
		struct relocation_info *reloc =
			(struct relocation_info *)((uintptr_t)lib->map +
				lib->macho_ext_reloc_off) + i;
		
		if (reloc->r_address == addr) {
			if (((uint32_t *)reloc)[1] == ((uint32_t *)&reloc_base)[1]) {
				return 1;
			} else if (((uint32_t *)reloc)[1] == ((uint32_t *)&reloc_si)[1]) {
				return 2;
			} else if (((uint32_t *)reloc)[1] == ((uint32_t *)&reloc_vmi)[1]) {
				return 3;
			}
		}
	}
	
	/*
	//uintptr_t search_begin = lib->data_off;
	//uintptr_t search_end   = lib->data_off + lib->data_size;
	uintptr_t search_begin = 0;
	uintptr_t search_end   = lib->size;
	
	pr_debug("FROM %08x\nTO   %08x\n", search_begin, search_end);
	
	for (uintptr_t i = search_begin; i < search_end -
		sizeof(struct relocation_info); i += 4) {
		if (memcmp((uint8_t *)lib->map + i, &reloc_base,
			sizeof(struct relocation_info)) == 0) {
			return 1;
		}
		if (memcmp((uint8_t *)lib->map + i, &reloc_si,
			sizeof(struct relocation_info)) == 0) {
			return 2;
		}
		if (memcmp((uint8_t *)lib->map + i, &reloc_vmi,
			sizeof(struct relocation_info)) == 0) {
			pr_debug("found reloc entry @ %08x\n", i);
			for (int j = 0; j < lib->macho_sect_count; ++j) {
				struct section *sect = lib->macho_sects[j];
				
				if (i >= sect->addr && i < sect->addr + sect->size) {
					pr_debug("found reloc entry in memory space of section:\n");
					pr_debug("seg '%s' sect '%s'\n",
						sect->segname, sect->sectname);
				}
				if (i >= sect->offset && i < sect->offset + sect->size) {
					pr_debug("found reloc entry in file space of section:\n");
					pr_debug("seg '%s' sect '%s'\n",
						sect->segname, sect->sectname);
				}
			}
			
			return 3;
		}
	}*/
	
	return 0;
}
