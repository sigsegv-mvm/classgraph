#include "all.h"


static struct nlist *macho_get_sym_by_index(library_info_t *lib, int index)
{
	assert(index < lib->macho_symtab_count);
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


static const char *macho_get_lc_str(struct load_command *cmd, union lc_str str)
{
	assert(str.offset + 1 < cmd->cmdsize);
	return (const char *)cmd + str.offset;
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
	
	assert(lib->macho_hdr->filetype == MH_DYLIB || lib->macho_hdr->filetype == MH_EXECUTE);
	
	//pr_debug("ncmds:      %08x\n", lib->macho_hdr->ncmds);
	//pr_debug("sizeofcmds: %08x\n", lib->macho_hdr->sizeofcmds);
	//pr_debug("flags:      %08x\n", lib->macho_hdr->flags);
	
	
	uint32_t nsegs = 0;
	struct segment_command *segs[16];
	
	lib->macho_vmrange_count = 0;
	
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
		
		switch (cmd->cmd & ~LC_REQ_DYLD) {
		case LC_SEGMENT:
		{
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
			
			assert(seg_cmd->fileoff + seg_cmd->filesize <= lib->size);
			
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
				
				assert(lib->macho_vmrange_count < 256);
				macho_vmrange_t *vmrange = lib->macho_vmranges + lib->macho_vmrange_count++;
				
				vmrange->is_mapped = (seg_cmd->filesize != 0 && ((sect->flags & SECTION_TYPE) != S_ZEROFILL));
				
				assert(sect->addr              >= seg_cmd->vmaddr);
				assert(sect->addr              <= seg_cmd->vmaddr + seg_cmd->vmsize);
				assert(sect->addr + sect->size <= seg_cmd->vmaddr + seg_cmd->vmsize);
				vmrange->vm_addr_begin = sect->addr;
				vmrange->vm_addr_end   = sect->addr + sect->size;
				
				assert(!vmrange->is_mapped || sect->offset              >= seg_cmd->fileoff);
				assert(!vmrange->is_mapped || sect->offset              <= seg_cmd->fileoff + seg_cmd->filesize);
				assert(!vmrange->is_mapped || sect->offset + sect->size <= seg_cmd->fileoff + seg_cmd->filesize);
				vmrange->map_off_begin = sect->offset;
				vmrange->map_off_end   = sect->offset + sect->size;
				
				vmrange->p_segment = seg_cmd;
				vmrange->p_section = sect;
				
				vmrange->segment_idx = nsegs;
				vmrange->section_idx = j;
				
				/* verify that no other vmranges overlap with this one */
				uint32_t vmrange1_lo = vmrange->vm_addr_begin;
				uint32_t vmrange1_hi = vmrange->vm_addr_end;
				for (int k = 0; k < lib->macho_vmrange_count - 1; ++k) {
					uint32_t vmrange2_lo = lib->macho_vmranges[k].vm_addr_begin;
					uint32_t vmrange2_hi = lib->macho_vmranges[k].vm_addr_end;
					
					assert(vmrange1_hi <= vmrange2_lo || vmrange2_hi <= vmrange1_lo);
				}
				
				sect_addr += sizeof(struct section);
			}
			
			assert(nsegs < 16);
			segs[nsegs++] = seg_cmd;
			
			break;
		}
		case LC_SYMTAB:
		{
			struct symtab_command *symtab_cmd = (struct symtab_command *)cmd;
			
			//pr_debug("  symoff:    %08x\n", symtab_cmd->symoff);
			//pr_debug("  nsyms:     %08x\n", symtab_cmd->nsyms);
			//pr_debug("  stroff:    %08x\n", symtab_cmd->stroff);
			//pr_debug("  strsize:   %08x\n", symtab_cmd->strsize);
			
			lib->macho_symtab_off   = symtab_cmd->symoff;
			lib->macho_symtab_count = symtab_cmd->nsyms;
			lib->macho_strtab_off   = symtab_cmd->stroff;
			lib->macho_strtab_size  = symtab_cmd->strsize;
			
			break;
		}
		case LC_SYMSEG: break; // TODO
		case LC_THREAD:
		case LC_UNIXTHREAD:
		{
			uint32_t *ptr = (uint32_t *)(cmd_addr + sizeof(struct thread_command));
			
			//while ((uintptr_t)ptr + 8 <= cmd_addr + cmd->cmdsize) {
			//	uint32_t flavor = *(ptr++);
			//	pr_debug("  flavor:    %08x\n", flavor);
			//	
			//	uint32_t count = *(ptr++);
			//	pr_debug("  state:     ");
			//	if (count != 0) {
			//		bool first = true;
			//		while (count-- != 0 && (uintptr_t)ptr + 4 <= cmd_addr + cmd->cmdsize) {
			//			pr_debug("%*s%08x\n", (first ? 0 : 13), "", *(ptr++));
			//			first = false;
			//		}
			//	} else {
			//		pr_debug("<none>\n");
			//	}
			//}
			
			break;
		}
		case LC_LOADFVMLIB: break; // TODO
		case LC_IDFVMLIB:   break; // TODO
		case LC_IDENT:      break; // TODO
		case LC_FVMFILE:    break; // TODO
		case LC_PREPAGE:    break; // TODO
		case LC_DYSYMTAB:
		{
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
			
			break;
		}
		case LC_LOAD_DYLIB:
		case LC_ID_DYLIB:
		case LC_LOAD_WEAK_DYLIB:
		case LC_REEXPORT_DYLIB:
		{
			struct dylib_command *dylib_cmd = (struct dylib_command *)cmd;
			
			//pr_debug("  dylib.name:                  \"%s\"\n", macho_get_lc_str(cmd, dylib_cmd->dylib.name));
			//pr_debug("  dylib.timestamp:             %08x\n",   dylib_cmd->dylib.timestamp);
			//pr_debug("  dylib.current_version:       %08x\n",   dylib_cmd->dylib.current_version);
			//pr_debug("  dylib.compatibility_version: %08x\n",   dylib_cmd->dylib.compatibility_version);
			
			break;
		}
		case LC_LOAD_DYLINKER:
		case LC_ID_DYLINKER:
		case LC_DYLD_ENVIRONMENT:
		{
			struct dylinker_command *dylinker_cmd = (struct dylinker_command *)cmd;
			
			//pr_debug("  name:      \"%s\"\n", macho_get_lc_str(cmd, dylinker_cmd->name));
			
			break;
		}
		case LC_PREBOUND_DYLIB:
		{
			struct prebound_dylib_command *prebounddylib_cmd = (struct prebound_dylib_command *)cmd;
			
			//pr_debug("  name:               \"%s\"\n", macho_get_lc_str(cmd, prebounddylib_cmd->name));
			//pr_debug("  nmodules:           %08x\n", prebounddylib_cmd->nmodules);
			
			//uint8_t *linked_modules = (uint8_t *)macho_get_lc_str(cmd, prebounddylib_cmd->linked_modules);
			//for (uint32_t j = 0; j < prebounddylib_cmd->nmodules; ++j) {
			//	uint8_t is_bound = ((linked_modules[j / 8] >> (j % 8)) & 0x01);
			//	pr_debug("  linked_modules[%02x]: %s\n", j, (is_bound ? "BOUND" : "not bound"));
			//}
			
			break;
		}
		case LC_ROUTINES:      break; // TODO
		case LC_SUB_FRAMEWORK: break; // TODO
		case LC_SUB_UMBRELLA:  break; // TODO
		case LC_SUB_CLIENT:    break; // TODO
		case LC_SUB_LIBRARY:   break; // TODO
		case LC_TWOLEVEL_HINTS:
		{
			struct twolevel_hints_command *twolevelhints_cmd = (struct twolevel_hints_command *)cmd;
			
			//pr_debug("  offset:    %08x\n", twolevelhints_cmd->offset);
			//pr_debug("  nhints:    %08x\n", twolevelhints_cmd->nhints);
			
			break;
		}
		case LC_PREBIND_CKSUM:
		{
			struct prebind_cksum_command *prebindcksum_cmd = (struct prebind_cksum_command *)cmd;
			
			//pr_debug("  cksum:     %08x\n", prebindcksum_cmd->cksum);
			
			break;
		}
		case LC_SEGMENT_64:         break; // TODO
		case LC_ROUTINES_64:        break; // TODO
		case LC_UUID:               break; // TODO
		case LC_RPATH:              break; // TODO
		case LC_CODE_SIGNATURE:     break; // TODO
		case LC_SEGMENT_SPLIT_INFO: break; // TODO
		case LC_LAZY_LOAD_DYLIB:    break; // TODO
		case LC_ENCRYPTION_INFO:    break; // TODO
		case LC_DYLD_INFO:
		case LC_DYLD_INFO_ONLY:
		{
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
			
			break;
		}
		case LC_LOAD_UPWARD_DYLIB:        break; // TODO
		case LC_VERSION_MIN_MACOSX:       break; // TODO
		case LC_VERSION_MIN_IPHONEOS:     break; // TODO
		case LC_FUNCTION_STARTS:          break; // TODO
		case LC_MAIN:                     break; // TODO
		case LC_DATA_IN_CODE:             break; // TODO
		case LC_SOURCE_VERSION:           break; // TODO
		case LC_DYLIB_CODE_SIGN_DRS:      break; // TODO
		case LC_ENCRYPTION_INFO_64:       break; // TODO
		case LC_LINKER_OPTION:            break; // TODO
		case LC_LINKER_OPTIMIZATION_HINT: break; // TODO
		case LC_VERSION_MIN_TVOS:         break; // TODO
		case LC_VERSION_MIN_WATCHOS:      break; // TODO
		}
		
		cmd_addr += cmd->cmdsize;
	}
	
	if (lib->macho_has_dyld_info) {
		const uint8_t *start = (const uint8_t *)((uintptr_t)lib->map + lib->macho_bind_off);
		const uint8_t *end   = start + lib->macho_bind_size;
		get_dyld_bind_info(start, end, NULL, 0, segs, nsegs, NULL, 0, &lib->macho_dbi, &lib->macho_ndbi);
		
	//	print_dyld_bind_info(lib->macho_dbi, lib->macho_ndbi);
	}
	
	/* debugging: print VM range information */
	//pr_debug("%-32s  %17s %-8s  %17s %-8s\n", "NAME", "VM_ADDR", "VM_SIZE", "MAP_OFF", "MAP_SIZE");
	//struct segment_command *last_seg = NULL;
	//for (int i = 0; i < lib->macho_vmrange_count; ++i) {
	//	macho_vmrange_t *vmrange = lib->macho_vmranges + i;
	//	
	//	struct segment_command *seg = vmrange->p_segment;
	//	if (seg != last_seg) {
	//		pr_debug("\n%-32s  %08x-%08x %08x  %08x-%08x %08x\n",
	//			seg->segname,
	//			seg->vmaddr,
	//			(seg->vmaddr + seg->vmsize) - 1,
	//			seg->vmsize,
	//			seg->fileoff,
	//			(seg->fileoff + seg->filesize) - 1,
	//			seg->filesize);
	//		
	//		last_seg = seg;
	//	}
	//	
	//	pr_debug("  %-30s  %08x-%08x %08x  %08x-%08x %08x\n",
	//		vmrange->p_section->sectname,
	//		vmrange->vm_addr_begin,
	//		vmrange->vm_addr_end - 1,
	//		(vmrange->vm_addr_end - vmrange->vm_addr_begin),
	//		vmrange->map_off_begin,
	//		vmrange->map_off_end - 1,
	//		(vmrange->map_off_end - vmrange->map_off_begin));
	//}
}


static const char *macho_sym_type_string(uint8_t n_type)
{
	switch ((n_type & N_TYPE)) {
	case N_UNDF: return "N_UNDF";
	case N_ABS:  return "N_ABS";
	case 0x4:    return "0x4";
	case 0x6:    return "0x6";
	case 0x8:    return "0x8";
	case N_INDR: return "N_INDR";
	case N_PBUD: return "N_PBUD";
	case N_SECT: return "N_SECT";
	default:     return "???";
	}
}

void symtab_macho_foreach(library_info_t *lib, void (*callback)(const symbol_t *))
{
	uintptr_t sym_off = (uintptr_t)lib->map + lib->macho_symtab_off;
	for (int i = 0; i < lib->macho_symtab_count; ++i) {
		struct nlist *sym = (struct nlist *)sym_off + i;
		
		/* sym->n_type sub-fields */
		uint8_t n_stab = (sym->n_type & N_STAB);
		uint8_t n_pext = (sym->n_type & N_PEXT);
		uint8_t n_type = (sym->n_type & N_TYPE);
		uint8_t n_ext  = (sym->n_type & N_EXT);
		
		//pr_debug("SYMBOL %d\n", i);
		//pr_debug("  n_strx:  %08x\n", sym->n_un.n_strx);
		//pr_debug("  n_type:  %02x [stab:%c] [ext:%c] [pext:%c] [type:%s]\n", sym->n_type,
		//	(n_stab != 0 ? 'Y' : 'N'),
		//	(n_ext  != 0 ? 'Y' : 'N'),
		//	(n_pext != 0 ? 'Y' : 'N'),
		//	macho_sym_type_string(n_type));
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
		
		/* skip external and debug entries */
		if (n_ext != 0 || n_stab != 0) continue;
		
		/* we don't have handling for indirect symbols set up yet;
		 * see osx/mach-o/nlist.h */
		assert(n_type != N_INDR);
		
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
		
		/* if r_extern == 0, then r_symbolnum is NOT a symbol index, but rather a section ordinal
		 * if r_type != R_ABS, then it's some kind of weird relocation we don't know how to deal with
		 * if r_length != 2, then the relocation isn't for a 32-bit value ( 0 => 8-bit, 1 => 16-bit, 2 => 32-bit, 3 => 64-bit ) */
		if (reloc->r_address == addr && reloc->r_extern == 1 && reloc->r_type == R_ABS && reloc->r_length == 2) {
			//pr_debug("r_address:   %08x\n", reloc->r_address);
			//pr_debug("r_symbolnum: %06x\n", reloc->r_symbolnum);
			//pr_debug("r_pcrel:     %01x\n", reloc->r_pcrel);
			//pr_debug("r_length:    %01x\n", reloc->r_length);
			//pr_debug("r_extern:    %01x\n", reloc->r_extern);
			//pr_debug("r_type:      %01x\n", reloc->r_type);
			
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


/* convert a VM address to a pointer into the mapped file */
void *symtab_macho_get_ptr(library_info_t *lib, uintptr_t addr, size_t size)
{
	macho_vmrange_t *vmrange = NULL;
	
	for (int i = 0; i < lib->macho_vmrange_count; ++i) {
		if (addr >= lib->macho_vmranges[i].vm_addr_begin && addr < lib->macho_vmranges[i].vm_addr_end) {
			vmrange = lib->macho_vmranges + i;
			break;
		}
	}
	
	/* couldn't find a VM address range containing addr */
	assert(vmrange != NULL);
	
	/* found a VM address range containing addr, but it's a non-file-mapped range */
	assert(vmrange->is_mapped);
	
	/* ensure that the entire size fits within the VM address range */
	assert((addr + size) <= vmrange->vm_addr_end);
	
	uintptr_t map_off = vmrange->map_off_begin + (addr - vmrange->vm_addr_begin);
	return (void *)((uintptr_t)lib->map + map_off);
}
