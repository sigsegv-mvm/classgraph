#include "all.h"


//static const char *libs_wanted[] = {
//	"engine_srv.so",
//	"server_srv.so",
//	"libtier0_srv.so",
//	"soundemittersystem_srv.so",
//};

//#define NUM_LIBS (sizeof(libs_wanted) / sizeof(*libs_wanted))
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
	
	//pr_debug("lib_hook:\n  path '%s'\n  name '%s'\n  handle %08x\n",
	//	path, name, (uintptr_t)handle);
	
	library_info_t *lib = NULL;
	for (int i = 0; i < NUM_LIBS; ++i) {
		if (libs[i].name == NULL) {
			lib = libs + i;
			break;
		}
	}
	if (lib == NULL) {
		return;
	}
	
	//pr_debug("lib_hook:\n  using lib idx %d (NUM_LIBS = %d)\n",
	//	(lib - libs), NUM_LIBS);
	
	if (lib->name != NULL) {
		//pr_warn("%s: path '%s' hooked twice!\n", __func__, path);
		return;
	}
	
	
	lib->name = name;
	lib->path = path;
	
	if ((lib->fd = open(lib->path, O_RDONLY)) == -1) {
	/*	char *fixed_path = malloc(1024);
		snprintf(fixed_path, 1024, "../%s", path);
		
		if ((lib->fd = open(fixed_path, O_RDONLY)) == -1) {
			snprintf(fixed_path, 1024, "../../%s", path);
			
			if ((lib->fd = open(fixed_path, O_RDONLY)) == -1) {
				snprintf(fixed_path, 1024, "../../../%s", path);
				
				if ((lib->fd = open(fixed_path, O_RDONLY)) == -1) {*/
					err(1, "open('%s') failed", name);
	/*			}
			}
		}
		
		lib->path = fixed_path;*/
	}
	
	if ((lib->handle = dlopen(lib->path, RTLD_LAZY | RTLD_GLOBAL)) == NULL) {
		errx(1, "dlopen('%s') failed: %s", name, dlerror());
	}
	
	if (fstat(lib->fd, &lib->stat) != 0) {
		err(1, "fstat('%s') failed", name);
	}
	lib->size = lib->stat.st_size;
	
	//pr_debug("lib_hook:\n  path '%s'\n  fd %d\n  size %d\n",
	//	lib->path, lib->fd, lib->size);
	
	if ((lib->map = mmap(NULL, lib->size, PROT_READ, MAP_PRIVATE, lib->fd, 0))
		== MAP_FAILED) {
		err(1, "mmap('%s') failed", name);
	}
	
	if (dlinfo(lib->handle, RTLD_DI_LINKMAP, &lib->linkmap) != 0) {
		errx(1, "dlinfo('%s', RTLD_DI_LINKMAP) failed: %s", name, dlerror());
	}
	lib->baseaddr = lib->linkmap->l_addr;
	
	symtab_init(lib);
	
	/*if (strcmp(name, "server_srv.so") == 0) {
		symbols_init();
		detour_init.setup();
	}*/
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
