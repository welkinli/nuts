#ifndef	__MODULE_ALBERT_H_albert
#define	__MODULE_ALBERT_H_albert	1

#include	"common_albert.h"
#include	"program_utils.h"
#include	<dlfcn.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef	struct {
	void *handle;
} GModule_albert_t;

extern GModule_albert_t *g_module_open_albert(const char *module_name);
extern int g_module_close_albert(GModule_albert_t *module);
extern char *g_module_build_path_albert(const char *dir_name, const char *pure_name);
extern int g_module_symbol_albert(GModule_albert_t *module, const char *symbol_name, void **ref_addr);
extern char *g_module_error_albert(void);


#ifdef	__cplusplus
}
#endif

#endif

