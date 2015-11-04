#ifndef _LIBTF2MOD_SYMTAB_H
#define _LIBTF2MOD_SYMTAB_H


void symtab_init(library_info_t *lib);

void symtab_foreach(void (*callback)(const symbol_t *));

bool symtab_lookup_addr(library_info_t *lib, symbol_t *entry, uintptr_t addr);

bool symtab_addr_abs(symbol_t *entry, uintptr_t addr);


#endif
