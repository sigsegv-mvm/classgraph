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
	/* fundamental stuff */
	bool is_primary;
	const char *name;
	const char *path;
	bool is_elf;
	bool is_macho;
	
	/* generic file stuff */
	int         fd;
	struct stat stat;
	size_t      size;
	void       *map;
	
	/* libdl stuff */
	void            *handle;
	struct link_map *linkmap;
	uintptr_t        baseaddr;
	
	/* library dependency stuff */
	int depend_count;
	const char *depends[256];
	
	/* elf stuff */
	Elf        *elf;
	Elf32_Ehdr *elf_hdr;
	GElf_Shdr   elf_symtab_shdr;
	Elf_Data   *elf_symtab_data;
	int         elf_symtab_count;
	
	/* mach-o stuff */
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


#define ANSI_RESET           0
#define ANSI_FG_BLACK       30
#define ANSI_FG_RED         31
#define ANSI_FG_GREEN       32
#define ANSI_FG_YELLOW      33
#define ANSI_FG_BLUE        34
#define ANSI_FG_MAGENTA     35
#define ANSI_FG_CYAN        36
#define ANSI_FG_WHITE       37
#define ANSI_BG_BLACK       40
#define ANSI_BG_RED         41
#define ANSI_BG_GREEN       42
#define ANSI_BG_YELLOW      43
#define ANSI_BG_BLUE        44
#define ANSI_BG_MAGENTA     45
#define ANSI_BG_CYAN        46
#define ANSI_BG_WHITE       47
#define ANSI_FG_BR_BLACK    90
#define ANSI_FG_BR_RED      91
#define ANSI_FG_BR_GREEN    92
#define ANSI_FG_BR_YELLOW   93
#define ANSI_FG_BR_BLUE     94
#define ANSI_FG_BR_MAGENTA  95
#define ANSI_FG_BR_CYAN     96
#define ANSI_FG_BR_WHITE    97
#define ANSI_BG_BR_BLACK   100
#define ANSI_BG_BR_RED     101
#define ANSI_BG_BR_GREEN   102
#define ANSI_BG_BR_YELLOW  103
#define ANSI_BG_BR_BLUE    104
#define ANSI_BG_BR_MAGENTA 105
#define ANSI_BG_BR_CYAN    106
#define ANSI_BG_BR_WHITE   107


#define _c_str_(_x) #_x
#define _c_str(_x) _c_str_(_x)
#define _c_ansistr(_x) _c_str(ANSI_##_x)
#define _c_fg(_clr, _str) "\e[" _c_ansistr(FG_##_clr) "m" _str "\e[" _c_ansistr(RESET) "m"
#define _c_bg(_clr, _str) "\e[" _c_ansistr(BG_##_clr) "m" _str "\e[" _c_ansistr(RESET) "m"


#define warn(         _fmt, ...) warn(         _c_fg(BR_YELLOW, _fmt), ##__VA_ARGS__)
#define warnx(        _fmt, ...) warnx(        _c_fg(BR_YELLOW, _fmt), ##__VA_ARGS__)
#define err(   _code, _fmt, ...) err(   _code, _c_fg(BR_RED   , _fmt), ##__VA_ARGS__)
#define errx(  _code, _fmt, ...) errx(  _code, _c_fg(BR_RED   , _fmt), ##__VA_ARGS__)


#define _pr_common(_clr, _fmt, ...) \
	fflush(stdout); fprintf(stderr, _c_fg(_clr, _fmt), ##__VA_ARGS__); fflush(stderr)

#define pr_debug(  _fmt, ...) _pr_common(   WHITE , _fmt, ##__VA_ARGS__)
#define pr_info(   _fmt, ...) _pr_common(BR_WHITE , _fmt, ##__VA_ARGS__)
#define pr_warn(   _fmt, ...) _pr_common(BR_YELLOW, _fmt, ##__VA_ARGS__)
#define pr_err(    _fmt, ...) _pr_common(BR_RED   , _fmt, ##__VA_ARGS__)
#define pr_special(_fmt, ...) _pr_common(BR_CYAN  , _fmt, ##__VA_ARGS__)


#endif
