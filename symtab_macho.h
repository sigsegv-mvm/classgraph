#ifndef _LIBTF2MOD_SYMTAB_MACHO_H
#define _LIBTF2MOD_SYMTAB_MACHO_H


void symtab_macho_init(library_info_t *lib);

void symtab_macho_foreach(library_info_t *lib, void (*callback)(const symbol_t *));

bool symtab_macho_find_reloc_sym_for_addr(library_info_t *lib, symbol_t *entry, uintptr_t addr);


#endif
