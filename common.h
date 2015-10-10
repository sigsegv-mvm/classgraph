#ifndef _LIBTF2MOD_COMMON_H
#define _LIBTF2MOD_COMMON_H


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
	
	/* elf symtab stuff */
	Elf        *elf;
	Elf32_Ehdr *elf_hdr;
	Elf_Data   *symtab_data;
	GElf_Shdr   symtab_shdr;
	int         symtab_count;
	
	/* section info */
	uintptr_t text_off;
	size_t    text_size;
	uintptr_t data_off;
	size_t    data_size;
	uintptr_t rodata_off;
	size_t    rodata_size;
	uintptr_t datarelro_off;
	size_t    datarelro_size;
	uintptr_t bss_off;
	size_t    bss_size;
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
