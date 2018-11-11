#include "all.h"


#define NUM_LIBS 256
static library_info_t libs[NUM_LIBS];


void lib_init(bool primary, const char *path)
{
	char *path_dir  = strdup(path);
	char *path_base = strdup(path);
	
	path_dir  = dirname(path_dir);
	path_base = basename(path_base);
	
	pr_debug("lib_init: '%s'\n", path);
	
	library_info_t *lib = NULL;
	for (int i = 0; i < NUM_LIBS; ++i) {
		if (libs[i].name == NULL) {
			lib = libs + i;
			break;
		}
	}
	assert(lib != NULL);
	
	lib->is_primary = primary;
	
	lib->name = path_base;
	lib->path = path;
	
	// HACK: try /usr/lib32/ prefix if relative open() fails
	if ((lib->fd = open(lib->path, O_RDONLY)) == -1) {
		char path2[4096];
		strlcpy(path2, "/usr/lib32/", sizeof(path2));
		strlcat(path2, path, sizeof(path2));
		
		if ((lib->fd = open(path2, O_RDONLY)) == -1) {
			err(1, "open('%s') failed", path2);
		}
		
		lib->path = strdup(path2);
	}
	
	if (fstat(lib->fd, &lib->stat) != 0) {
		err(1, "fstat('%s') failed", lib->path);
	}
	lib->size = lib->stat.st_size;
	
	if ((lib->map = mmap(NULL, lib->size, PROT_READ, MAP_PRIVATE, lib->fd, 0)) == MAP_FAILED) {
		err(1, "mmap('%s') failed", lib->path);
	}
	
	if (lib->size >= EI_NIDENT) {
		const char *e_ident = lib->map;
		if (e_ident[EI_MAG0] == ELFMAG0 && e_ident[EI_MAG1] == ELFMAG1 && e_ident[EI_MAG2] == ELFMAG2 && e_ident[EI_MAG3] == ELFMAG3) {
			assert(e_ident[EI_CLASS]   == ELFCLASS32);
			assert(e_ident[EI_DATA]    == ELFDATA2LSB);
			assert(e_ident[EI_VERSION] == EV_CURRENT);
			
			pr_debug("lib_init('%s'): detected ELF32\n", lib->path);
			lib->is_elf = true;
		}
	}
	
	if (lib->size >= sizeof(uint32_t)) {
		const uint32_t *magic = lib->map;
		if (*magic == MH_MAGIC) {
			pr_debug("lib_init('%s'): detected Mach-O (non-fat)\n", lib->path);
			lib->is_macho = true;
		} else if (*magic == FAT_CIGAM) {
			pr_debug("lib_init('%s'): detected Mach-O (fat)\n", lib->path);
			lib->is_macho = true;
		}
	}
	
	assert(lib->is_elf || lib->is_macho);
	assert(!(lib->is_elf && lib->is_macho));
	
	if (lib->is_elf) {
		if ((lib->handle = dlopen(lib->path, RTLD_NOW | RTLD_GLOBAL)) == NULL) {
			errx(1, "dlopen('%s') failed: %s", lib->path, dlerror());
		}
		
		if (dlinfo(lib->handle, RTLD_DI_LINKMAP, &lib->linkmap) != 0) {
			warnx("dlinfo('%s', RTLD_DI_LINKMAP) failed: %s", lib->path, dlerror());
			return;
		}
		
		lib->baseaddr = lib->linkmap->l_addr;
	} else {
		lib->baseaddr = 0;
	}
	
	symtab_init(lib);
	#warning TODO: lib->depends and lib->depend_count for mach-o!
	// TODO: in symtab_macho_init, find 'load_dylib' load commands: these contain the names of the needed libraries
	// (probably should run 'em through basename() or something)
	
	for (int i = 0; i < lib->depend_count; ++i) {
		if (lib_find(lib->depends[i]) == NULL) {
			lib_init(false, lib->depends[i]);
		}
	}
}


library_info_t *lib_find(const char *name)
{
	for (library_info_t *lib = libs; lib < libs + NUM_LIBS; ++lib) {
		if (!lib->is_primary && lib->name != NULL && strcasecmp(name, lib->name) == 0) {
			return lib;
		}
	}
	return NULL;
}


library_info_t *lib_first(void)
{
	return libs;
}

library_info_t *lib_next(library_info_t *lib)
{
	if (++lib < libs + NUM_LIBS && lib->name != NULL) {
		return lib;
	} else {
		return NULL;
	}
}
