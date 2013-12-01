#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN    "module_albert"
#endif

#include	"module_albert.h"
#include	"albert_log.h"

extern CRollLog *g_albert_logp;
extern uint32_t g_log_level;

GModule_albert_t *
g_module_open_albert(const char *module_name)
{
	GModule_albert_t *module = NULL;
	
	module = (GModule_albert_t *)dlmalloc(sizeof(GModule_albert_t));
	if (!module)
		return NULL;

	memset(module, 0, sizeof(GModule_albert_t));
	//module->handle = dlopen(module_name, RTLD_LAZY);
	module->handle = dlopen(module_name, RTLD_NOW);
	if (!(module->handle)) {
		free(module);
		return NULL;
	}
	
	return module;
}

int 
g_module_close_albert(GModule_albert_t *module)
{
	if (!module)
		return -1;
	
	dlclose(module->handle);
	free(module);

	return 0;
}
	
char *
g_module_build_path_albert(const char *dir_name, const char *pure_name)
{
	uint32_t dir_name_len = 0, pure_name_len = 0;
	char *result_str = NULL;

	if (!dir_name || !pure_name)
		return NULL;
	
	if (dir_name)
		dir_name_len = strlen(dir_name);

	if (pure_name)
		pure_name_len = strlen(pure_name);

	if (dir_name_len + pure_name_len == 0)
		return NULL;
	
	result_str = (char *)dlmalloc(dir_name_len + pure_name_len + 32);
	if (!result_str)
		return NULL;

	memset(result_str, 0, dir_name_len + pure_name_len + 32);
	snprintf(result_str, dir_name_len + pure_name_len + 31, "%s/lib%s.so", dir_name, pure_name);
	
	return result_str;
}

int 
g_module_symbol_albert(GModule_albert_t *module, const char *symbol_name, void **ref_addr)
{
	void *ref = NULL;
	
	if (!module || !ref_addr)
		return -1;

	ref = dlsym(module->handle, (const char *)symbol_name);
	if (!ref)
		return -2;
	
	*ref_addr = ref;
	
	return 0;
}

char *
g_module_error_albert(void)
{
	return dlerror();
}

