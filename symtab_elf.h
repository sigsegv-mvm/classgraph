#ifndef _LIBTF2MOD_SYMTAB_ELF_H
#define _LIBTF2MOD_SYMTAB_ELF_H


void symtab_elf_init(library_info_t *lib);

void symtab_elf_foreach(library_info_t *lib, void (*callback)(const symbol_t *));


#endif
