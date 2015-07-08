#ifndef FK_PLUGIN_C
#define FK_PLUGIN_C
#include "input.h"
#include <dlfcn.h>
void* load_plugin(char* plugin_prefix, char* plugin_name)
{
    char*  lib = malloc(strlen(plugin_prefix) + strlen(plugin_name) + 4);
    strcpy(lib, plugin_prefix);
    strcpy(lib + strlen(plugin_prefix), plugin_name);
    strcpy(lib + strlen(plugin_prefix) + strlen(plugin_name), ".so");
    trace_debug("load_plugin_function: trying to open %s", lib);
    void* handle = dlopen(lib, RTLD_LAZY);
    free(lib);
    if(handle == NULL)
    {
        printf("%s\n", dlerror());
    }
    return handle;
}
void* load_function_from_plugin(void* handle, char* function_name)
{
    void* f = NULL;
    if(handle != NULL)
    {
        trace_debug("load_plugin_function: loading %s()", function_name);
        f = dlsym(handle, function_name);
        if(f == NULL)
        {
            trace_debug("load_plugin_function: loading setup function failed");
            printf("%s\n", dlerror());
        }
    }
    return f;
}
void* load_plugin_function(char* plugin_prefix, char* plugin_name, char* function_name)
{
    void* handle = load_plugin(plugin_prefix, plugin_name);
    load_function_from_plugin(handle, function_name);
}
#endif
