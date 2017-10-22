#include "all.h"


void symtab_init(library_info_t *lib)
{
	if (lib->is_elf) {
		symtab_elf_init(lib);
	}
	if (lib->is_macho) {
		symtab_macho_init(lib);
	}
}


void symtab_foreach(void (*callback)(const symbol_t *))
{
	library_info_t *lib = lib_first();
	
	do
	{
		if (lib->is_elf) {
			symtab_elf_foreach(lib, callback);
		}
		if (lib->is_macho) {
			symtab_macho_foreach(lib, callback);
		}
	} while ((lib = lib_next(lib)) != NULL);
}


bool symtab_addr_abs(symbol_t *entry, uintptr_t addr)
{
	library_info_t *lib = lib_first();
	
	do
	{
		if (addr >= lib->baseaddr && symtab_lookup_addr(lib, entry, addr - lib->baseaddr)) {
			return true;
		}
	} while ((lib = lib_next(lib)) != NULL);
	
	return false;
}
