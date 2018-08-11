#ifndef _LIBTF2MOD_COMMON_H
#define _LIBTF2MOD_COMMON_H


typedef struct {
	bool is_mapped; // is it even valid to access this VM range through the file mapping?
	
	uint32_t vm_addr_begin;
	uint32_t vm_addr_end;
	
	uint32_t map_off_begin;
	uint32_t map_off_end;
	
	struct segment_command *p_segment; // ptr to original LC_SEGMENT base
	struct section         *p_section; // ptr to original LC_SEGMENT struct section
	
	uint32_t segment_idx; // zero-based index of which LC_SEGMENT we are from
	uint32_t section_idx; // zero-based index of which LC_SEGMENT section we are from
} macho_vmrange_t;


typedef struct {
	const char *name;
	const char *path;
	
	/* generic file stuff */
	int         fd;
	struct stat stat;
	size_t      size;
	void       *map;
	
	/* libdl stuff */
	void            *handle;
	struct link_map *linkmap;
	uintptr_t        baseaddr;
	
	/* generic symtab stuff */
	bool is_elf;
	bool is_macho;
	
	/* elf symtab stuff */
	Elf        *elf;
	Elf32_Ehdr *elf_hdr;
	Elf_Data   *elf_symtab_data;
	GElf_Shdr   elf_symtab_shdr;
	int         elf_symtab_count;
	
	/* mach-o symtab stuff */
	struct mach_header    *macho_hdr;
	uint32_t               macho_symtab_off;      // LC_SYMTAB
	uint32_t               macho_symtab_count;    // LC_SYMTAB
	uint32_t               macho_strtab_off;      // LC_SYMTAB
	uint32_t               macho_strtab_size;     // LC_SYMTAB
	uint32_t               macho_ext_reloc_off;   // LC_DYSYMTAB
	uint32_t               macho_ext_reloc_count; // LC_DYSYMTAB
	bool                   macho_has_dyld_info;
	uint32_t               macho_rebase_off;      // LC_DYLD_INFO
	uint32_t               macho_rebase_size;     // LC_DYLD_INFO
	uint32_t               macho_bind_off;        // LC_DYLD_INFO
	uint32_t               macho_bind_size;       // LC_DYLD_INFO
	uint32_t               macho_weak_bind_off;   // LC_DYLD_INFO
	uint32_t               macho_weak_bind_size;  // LC_DYLD_INFO
	uint32_t               macho_lazy_bind_off;   // LC_DYLD_INFO
	uint32_t               macho_lazy_bind_size;  // LC_DYLD_INFO
	uint32_t               macho_export_off;      // LC_DYLD_INFO
	uint32_t               macho_export_size;     // LC_DYLD_INFO
	uint64_t               macho_ndbi;
	struct dyld_bind_info *macho_dbi;
	
	/* mach-o VM address range information */
	int             macho_vmrange_count;
	macho_vmrange_t macho_vmranges[256];
} library_info_t;


typedef struct {
	library_info_t *lib;
	uintptr_t       addr;
	uintptr_t       size;
	const char     *name;
	const char     *name_demangled;
} symbol_t;


#define pr_debug(_fmt, ...) \
	fprintf(stderr, "\e[37m" _fmt "\e[0m", ##__VA_ARGS__); fflush(stderr)
#define pr_info(_fmt, ...) \
	fprintf(stderr, "\e[97m" _fmt "\e[0m", ##__VA_ARGS__); fflush(stderr)
#define pr_warn(_fmt, ...) \
	fprintf(stderr, "\e[93m" _fmt "\e[0m", ##__VA_ARGS__); fflush(stderr)
#define pr_err(_fmt, ...) \
	fprintf(stderr, "\e[91m" _fmt "\e[0m", ##__VA_ARGS__); fflush(stderr)
#define pr_special(_fmt, ...) \
	fprintf(stderr, "\e[96m" _fmt "\e[0m", ##__VA_ARGS__); fflush(stderr)

#define warn(_fmt, ...) \
	warn("\e[93m" _fmt "\e[0m", ##__VA_ARGS__)
#define warnx(_fmt, ...) \
	warnx("\e[93m" _fmt "\e[0m", ##__VA_ARGS__)
#define err(_code, _fmt, ...) \
	err(_code, "\e[91m" _fmt "\e[0m", ##__VA_ARGS__)
#define errx(_code, _fmt, ...) \
	errx(_code, "\e[91m" _fmt "\e[0m", ##__VA_ARGS__)


#endif
