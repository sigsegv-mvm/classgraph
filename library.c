#include "all.h"


#define NUM_LIBS 100

static library_info_t libs[NUM_LIBS];


void lib_init(const char *path)
{
	char *path_dir  = strdup(path);
	char *path_base = strdup(path);
	
	path_dir  = dirname(path_dir);
	path_base = basename(path_base);
	
	const char *name = path_base;
	
	pr_debug("lib_init: '%s'\n", path);
	
	library_info_t *lib = NULL;
	for (int i = 0; i < NUM_LIBS; ++i) {
		if (libs[i].name == NULL) {
			lib = libs + i;
			break;
		}
	}
	assert(lib != NULL);
	
	lib->name = name;
	lib->path = path;
	
	if ((lib->fd = open(lib->path, O_RDONLY)) == -1) {
		err(1, "open('%s') failed", name);
	}
	
	if (fstat(lib->fd, &lib->stat) != 0) {
		err(1, "fstat('%s') failed", name);
	}
	lib->size = lib->stat.st_size;
	
	if ((lib->map = mmap(NULL, lib->size, PROT_READ, MAP_PRIVATE, lib->fd, 0)) == MAP_FAILED) {
		err(1, "mmap('%s') failed", name);
	}
	
	if (lib->size >= EI_NIDENT) {
		const char *e_ident = lib->map;
		if (e_ident[EI_MAG0] == ELFMAG0 && e_ident[EI_MAG1] == ELFMAG1 && e_ident[EI_MAG2] == ELFMAG2 && e_ident[EI_MAG3] == ELFMAG3) {
			assert(e_ident[EI_CLASS]   == ELFCLASS32);
			assert(e_ident[EI_DATA]    == ELFDATA2LSB);
			assert(e_ident[EI_VERSION] == EV_CURRENT);
			
			pr_debug("lib_init: detected ELF32\n");
			lib->is_elf = true;
		}
	}
	
	if (lib->size >= sizeof(uint32_t)) {
		const uint32_t *magic = lib->map;
		if (*magic == MH_MAGIC) {
			pr_debug("lib_init: detected Mach-O (non-fat)\n");
			lib->is_macho = true;
		} else if (*magic == FAT_CIGAM) {
			pr_debug("lib_init: detected Mach-O (fat)\n");
			lib->is_macho = true;
		}
	}
	
	assert(lib->is_elf || lib->is_macho);
	assert(!(lib->is_elf && lib->is_macho));
	
	if (lib->is_elf) {
		if ((lib->handle = dlopen(lib->path, RTLD_LAZY | RTLD_GLOBAL)) == NULL) {
			warnx("dlopen('%s') failed: %s", name, dlerror());
			return;
		}
		
		if (dlinfo(lib->handle, RTLD_DI_LINKMAP, &lib->linkmap) != 0) {
			warnx("dlinfo('%s', RTLD_DI_LINKMAP) failed: %s", name, dlerror());
			return;
		}
		
		// REMOVE ME
		const char *s;
		s = "_ZTI19IBaseObjectAutoList";
		pr_debug("dlsym('%s') = %08X\n", s, (uintptr_t)dlsym(lib->handle, s));
		s = "_ZN19IBaseObjectAutoListD0Ev";
		pr_debug("dlsym('%s') = %08X\n", s, (uintptr_t)dlsym(lib->handle, s));
		//exit(0);
		
		lib->baseaddr = lib->linkmap->l_addr;
	} else {
		lib->baseaddr = 0;
	}
	
	symtab_init(lib);
}


library_info_t *lib_find(const char *name)
{
	for (library_info_t *lib = libs; lib != libs + NUM_LIBS; ++lib) {
		if (lib->name != NULL && strcasecmp(name, lib->name) == 0) {
			return lib;
		}
	}
	return NULL;
}


library_info_t *lib_first(void)
{
	library_info_t *lib = libs;
	while (lib->name == NULL) {
		lib = lib_next(lib);
	}
	
	return lib;
}

library_info_t *lib_next(library_info_t *lib)
{
	if (++lib != libs + NUM_LIBS) {
		if (lib->name == NULL) {
			return lib_next(lib);
		} else {
			return lib;
		}
	} else {
		return NULL;
	}
}
