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
		const char *name = (const char *)((uintptr_t)lib->map + lib->macho_strtab_off + sym->n_un.n_strx);
		
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
	
	/* hacky support for universal binaries */
	if (NXSwapInt(lib->macho_hdr->magic) == FAT_MAGIC) {
		//pr_debug("detected universal binary\n");
		
		struct fat_header *hdr = (struct fat_header *)lib->macho_hdr;
		
		bool found_i386 = false;
		uintptr_t arch_addr = (uintptr_t)lib->map + sizeof(struct fat_header);
		for (int i = 0; i < hdr->nfat_arch; ++i) {
			struct fat_arch *arch = (struct fat_arch *)arch_addr;
			
			//pr_debug(
			//	"fat_arch %d:\n"
			//	"  cputype    %08x\n"
			//	"  cpusubtype %08x\n"
			//	"  offset     %08x\n"
			//	"  size       %08x\n"
			//	"  align      %08x\n",
			//	i,
			//	NXSwapInt(arch->cputype),
			//	NXSwapInt(arch->cpusubtype),
			//	NXSwapInt(arch->offset),
			//	NXSwapInt(arch->size),
			//	NXSwapInt(arch->align));
			
			if (NXSwapInt(arch->cputype) == CPU_TYPE_I386 && NXSwapInt(arch->cpusubtype) == CPU_SUBTYPE_I386_ALL) {
				lib->map = (void *)((uintptr_t)lib->map + NXSwapInt(arch->offset));
				lib->macho_hdr = lib->map;
				lib->size = NXSwapInt(arch->size);
				
				found_i386 = true;
				break;
			}
			
			arch_addr += sizeof(struct fat_arch);
		}
		assert(found_i386);
	}
	
	assert(lib->macho_hdr->magic      == MH_MAGIC);
	assert(lib->macho_hdr->cputype    == CPU_TYPE_I386);
	assert(lib->macho_hdr->cpusubtype == CPU_SUBTYPE_I386_ALL);
	assert(lib->macho_hdr->filetype   == MH_DYLIB);
	
	//pr_debug("ncmds:      %08x\n", lib->macho_hdr->ncmds);
	//pr_debug("sizeofcmds: %08x\n", lib->macho_hdr->sizeofcmds);
	//pr_debug("flags:      %08x\n", lib->macho_hdr->flags);
	
	
	uint32_t nsegs = 0;
	struct segment_command *segs[16];
	
	lib->macho_sect_count = 0;
	
	uintptr_t cmd_addr = (uintptr_t)lib->map + sizeof(struct mach_header);
	for (int i = 0; i < lib->macho_hdr->ncmds; ++i) {
		struct load_command *cmd = (struct load_command *)cmd_addr;
		
		//const char *cmd_name = "???";
		//switch (cmd->cmd & ~LC_REQ_DYLD) {
		//case LC_SEGMENT:                  cmd_name = "LC_SEGMENT";                  break;
		//case LC_SYMTAB:                   cmd_name = "LC_SYMTAB";                   break;
		//case LC_SYMSEG:                   cmd_name = "LC_SYMSEG";                   break;
		//case LC_THREAD:                   cmd_name = "LC_THREAD";                   break;
		//case LC_UNIXTHREAD:               cmd_name = "LC_UNIXTHREAD";               break;
		//case LC_LOADFVMLIB:               cmd_name = "LC_LOADFVMLIB";               break;
		//case LC_IDFVMLIB:                 cmd_name = "LC_IDFVMLIB";                 break;
		//case LC_IDENT:                    cmd_name = "LC_IDENT";                    break;
		//case LC_FVMFILE:                  cmd_name = "LC_FVMFILE";                  break;
		//case LC_PREPAGE:                  cmd_name = "LC_PREPAGE";                  break;
		//case LC_DYSYMTAB:                 cmd_name = "LC_DYSYMTAB";                 break;
		//case LC_LOAD_DYLIB:               cmd_name = "LC_LOAD_DYLIB";               break;
		//case LC_ID_DYLIB:                 cmd_name = "LC_ID_DYLIB";                 break;
		//case LC_LOAD_DYLINKER:            cmd_name = "LC_LOAD_DYLINKER";            break;
		//case LC_ID_DYLINKER:              cmd_name = "LC_ID_DYLINKER";              break;
		//case LC_PREBOUND_DYLIB:           cmd_name = "LC_PREBOUND_DYLIB";           break;
		//case LC_ROUTINES:                 cmd_name = "LC_ROUTINES";                 break;
		//case LC_SUB_FRAMEWORK:            cmd_name = "LC_SUB_FRAMEWORK";            break;
		//case LC_SUB_UMBRELLA:             cmd_name = "LC_SUB_UMBRELLA";             break;
		//case LC_SUB_CLIENT:               cmd_name = "LC_SUB_CLIENT";               break;
		//case LC_SUB_LIBRARY:              cmd_name = "LC_SUB_LIBRARY";              break;
		//case LC_TWOLEVEL_HINTS:           cmd_name = "LC_TWOLEVEL_HINTS";           break;
		//case LC_PREBIND_CKSUM:            cmd_name = "LC_PREBIND_CKSUM";            break;
		//case LC_LOAD_WEAK_DYLIB:          cmd_name = "LC_LOAD_WEAK_DYLIB";          break;
		//case LC_SEGMENT_64:               cmd_name = "LC_SEGMENT_64";               break;
		//case LC_ROUTINES_64:              cmd_name = "LC_ROUTINES_64";              break;
		//case LC_UUID:                     cmd_name = "LC_UUID";                     break;
		//case LC_RPATH:                    cmd_name = "LC_RPATH";                    break;
		//case LC_CODE_SIGNATURE:           cmd_name = "LC_CODE_SIGNATURE";           break;
		//case LC_SEGMENT_SPLIT_INFO:       cmd_name = "LC_SEGMENT_SPLIT_INFO";       break;
		//case LC_REEXPORT_DYLIB:           cmd_name = "LC_REEXPORT_DYLIB";           break;
		//case LC_LAZY_LOAD_DYLIB:          cmd_name = "LC_LAZY_LOAD_DYLIB";          break;
		//case LC_ENCRYPTION_INFO:          cmd_name = "LC_ENCRYPTION_INFO";          break;
		//case LC_DYLD_INFO:                cmd_name = "LC_DYLD_INFO";                break;
		//case LC_DYLD_INFO_ONLY:           cmd_name = "LC_DYLD_INFO_ONLY";           break;
		//case LC_LOAD_UPWARD_DYLIB:        cmd_name = "LC_LOAD_UPWARD_DYLIB";        break;
		//case LC_VERSION_MIN_MACOSX:       cmd_name = "LC_VERSION_MIN_MACOSX";       break;
		//case LC_VERSION_MIN_IPHONEOS:     cmd_name = "LC_VERSION_MIN_IPHONEOS";     break;
		//case LC_FUNCTION_STARTS:          cmd_name = "LC_FUNCTION_STARTS";          break;
		//case LC_DYLD_ENVIRONMENT:         cmd_name = "LC_DYLD_ENVIRONMENT";         break;
		//case LC_MAIN:                     cmd_name = "LC_MAIN";                     break;
		//case LC_DATA_IN_CODE:             cmd_name = "LC_DATA_IN_CODE";             break;
		//case LC_SOURCE_VERSION:           cmd_name = "LC_SOURCE_VERSION";           break;
		//case LC_DYLIB_CODE_SIGN_DRS:      cmd_name = "LC_DYLIB_CODE_SIGN_DRS";      break;
		//case LC_ENCRYPTION_INFO_64:       cmd_name = "LC_ENCRYPTION_INFO_64";       break;
		//case LC_LINKER_OPTION:            cmd_name = "LC_LINKER_OPTION";            break;
		//case LC_LINKER_OPTIMIZATION_HINT: cmd_name = "LC_LINKER_OPTIMIZATION_HINT"; break;
		//case LC_VERSION_MIN_TVOS:         cmd_name = "LC_VERSION_MIN_TVOS";         break;
		//case LC_VERSION_MIN_WATCHOS:      cmd_name = "LC_VERSION_MIN_WATCHOS";      break;
		//}
		
		//pr_debug("load_command %d:\n", i);
		//pr_debug("  cmd:       %08x %s\n", cmd->cmd, cmd_name);
		//pr_debug("  cmdsize:   %08x\n", cmd->cmdsize);
		
		if (cmd->cmd == LC_SEGMENT) {
			struct segment_command *seg_cmd = (struct segment_command *)cmd;
			
			segs[nsegs++] = seg_cmd;
			
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
				
				//pr_debug("  section %d:\n", j);
				//pr_debug("    sectname:  '%s'\n", sect->sectname);
				//pr_debug("    segname:   '%s'\n", sect->segname);
				//pr_debug("    addr:      %08x\n", sect->addr);
				//pr_debug("    size:      %08x\n", sect->size);
				//pr_debug("    offset:    %08x\n", sect->offset);
				//pr_debug("    align:     %08x\n", sect->align);
				//pr_debug("    reloff:    %08x\n", sect->reloff);
				//pr_debug("    nreloc:    %08x\n", sect->nreloc);
				//pr_debug("    flags:     %08x\n", sect->flags);
				//pr_debug("    reserved1: %08x\n", sect->reserved1);
				//pr_debug("    reserved2: %08x\n", sect->reserved2);
				
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
			struct dysymtab_command *dysymtab_cmd = (struct dysymtab_command *)cmd;
			
			//pr_debug("  ilocalsym:      %08x\n", dysymtab_cmd->ilocalsym);
			//pr_debug("  nlocalsym:      %08x\n", dysymtab_cmd->nlocalsym);
			//pr_debug("  iextdefsym:     %08x\n", dysymtab_cmd->iextdefsym);
			//pr_debug("  nextdefsym:     %08x\n", dysymtab_cmd->nextdefsym);
			//pr_debug("  iundefsym:      %08x\n", dysymtab_cmd->iundefsym);
			//pr_debug("  nundefsym:      %08x\n", dysymtab_cmd->nundefsym);
			//pr_debug("  tocoff:         %08x\n", dysymtab_cmd->tocoff);
			//pr_debug("  ntoc:           %08x\n", dysymtab_cmd->ntoc);
			//pr_debug("  modtaboff:      %08x\n", dysymtab_cmd->modtaboff);
			//pr_debug("  nmodtab:        %08x\n", dysymtab_cmd->nmodtab);
			//pr_debug("  extrefsymoff:   %08x\n", dysymtab_cmd->extrefsymoff);
			//pr_debug("  nextrefsyms:    %08x\n", dysymtab_cmd->nextrefsyms);
			//pr_debug("  indirectsymoff: %08x\n", dysymtab_cmd->indirectsymoff);
			//pr_debug("  nindirectsyms:  %08x\n", dysymtab_cmd->nindirectsyms);
			//pr_debug("  extreloff:      %08x\n", dysymtab_cmd->extreloff);
			//pr_debug("  nextrel:        %08x\n", dysymtab_cmd->nextrel);
			//pr_debug("  locreloff:      %08x\n", dysymtab_cmd->locreloff);
			//pr_debug("  nlocrel:        %08x\n", dysymtab_cmd->nlocrel);
			
			lib->macho_ext_reloc_off   = dysymtab_cmd->extreloff;
			lib->macho_ext_reloc_count = dysymtab_cmd->nextrel;
		} else if (cmd->cmd == LC_DYLD_INFO || cmd->cmd == LC_DYLD_INFO_ONLY) {
			struct dyld_info_command *dyldinfo_cmd = (struct dyld_info_command *)cmd;
			
			//pr_debug("  rebase_off:     %08x\n", dyldinfo_cmd->rebase_off);
			//pr_debug("  rebase_size:    %08x\n", dyldinfo_cmd->rebase_size);
			//pr_debug("  bind_off:       %08x\n", dyldinfo_cmd->bind_off);
			//pr_debug("  bind_size:      %08x\n", dyldinfo_cmd->bind_size);
			//pr_debug("  weak_bind_off:  %08x\n", dyldinfo_cmd->weak_bind_off);
			//pr_debug("  weak_bind_size: %08x\n", dyldinfo_cmd->weak_bind_size);
			//pr_debug("  lazy_bind_off:  %08x\n", dyldinfo_cmd->lazy_bind_off);
			//pr_debug("  lazy_bind_size: %08x\n", dyldinfo_cmd->lazy_bind_size);
			//pr_debug("  export_off:     %08x\n", dyldinfo_cmd->export_off);
			//pr_debug("  export_size:    %08x\n", dyldinfo_cmd->export_size);
			
			lib->macho_has_dyld_info  = true;
			lib->macho_rebase_off     = dyldinfo_cmd->rebase_off;
			lib->macho_rebase_size    = dyldinfo_cmd->rebase_size;
			lib->macho_bind_off       = dyldinfo_cmd->bind_off;
			lib->macho_bind_size      = dyldinfo_cmd->bind_size;
			lib->macho_weak_bind_off  = dyldinfo_cmd->weak_bind_off;
			lib->macho_weak_bind_size = dyldinfo_cmd->weak_bind_size;
			lib->macho_lazy_bind_off  = dyldinfo_cmd->lazy_bind_off;
			lib->macho_lazy_bind_size = dyldinfo_cmd->lazy_bind_size;
			lib->macho_export_off     = dyldinfo_cmd->export_off;
			lib->macho_export_size    = dyldinfo_cmd->export_size;
		}
		
		cmd_addr += cmd->cmdsize;
	}
	
	if (lib->macho_has_dyld_info) {
		const uint8_t *start = (const uint8_t *)((uintptr_t)lib->map + lib->macho_bind_off);
		const uint8_t *end   = start + lib->macho_bind_size;
		get_dyld_bind_info(start, end, NULL, 0, segs, nsegs, NULL, 0, &lib->macho_dbi, &lib->macho_ndbi);
		
	//	print_dyld_bind_info(lib->macho_dbi, lib->macho_ndbi);
	}
}


void symtab_macho_foreach(library_info_t *lib, void (*callback)(const symbol_t *))
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
			.lib            = lib,
			.addr           = macho_get_sym_addr(lib, sym),
			.size           = 0,
			.name           = macho_get_sym_name(lib, sym),
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


/* given a particular address, find out if there is a relocation entry for that
 * address, and if so, return some information about the symbol being relocated
 * to the address */
bool symtab_macho_find_reloc_sym_for_addr(library_info_t *lib, symbol_t *entry, uintptr_t addr)
{
	/* try old reloc info first */
	for (int i = 0; i < lib->macho_ext_reloc_count; ++i) {
		struct relocation_info *reloc = (struct relocation_info *)((uintptr_t)lib->map + lib->macho_ext_reloc_off) + i;
		
		//pr_debug("r_address:   %08x\n", reloc->r_address);
		//pr_debug("r_symbolnum: %06x\n", reloc->r_symbolnum);
		//pr_debug("r_pcrel:     %01x\n", reloc->r_pcrel);
		//pr_debug("r_length:    %01x\n", reloc->r_length);
		//pr_debug("r_extern:    %01x\n", reloc->r_extern);
		//pr_debug("r_type:      %01x\n", reloc->r_type);
		
		if (reloc->r_address == addr) {
			struct nlist *sym = macho_get_sym_by_index(lib, reloc->r_symbolnum);
			
			entry->lib            = lib;
			entry->addr           = macho_get_sym_addr(lib, sym);
			entry->size           = 0;
			entry->name           = macho_get_sym_name(lib, sym);
			entry->name_demangled = try_demangle_noprefix(entry->name);
			
			return true;
		}
	}
	
	/* try new compressed dyld bind info next */
	if (lib->macho_has_dyld_info) {
		for (uint64_t i = 0; i < lib->macho_ndbi; ++i) {
			struct dyld_bind_info *info = lib->macho_dbi + i;
			
			if (info->address == addr) {
				entry->lib            = lib;
				entry->addr           = info->address + info->addend;
				entry->size           = 0;
				entry->name           = info->symbolname + (info->symbolname[0] == '_' ? 1 : 0);
				entry->name_demangled = try_demangle_noprefix(entry->name);
				
				return true;
			}
		}
	}
	
	return false;
}
