#include "config.h"
/** @brief convert a symbol to a string */
#define STR(s) #s
/** @brief check if name matches a configuration name, if so points
 * current_size pointer to the size of the matched configuration and
 * sets the address of the string array for that configuration to next
 * item in argv list */
#define CONFIG_CURRENT(expected) \
    if(!strcmp(name, STR(expected))) { \
        current_size = &(conf->expected ## _n); \
        conf->expected = argv + i + 1; \
    }
#include "util.c"
void add_fields_and_tags(config* conf)
{
    if(conf->fields_s != NULL) free(conf->fields_s);
    conf->fields_s = array_to_container_string(
            conf->fields, conf->fields_n, '{', '}', ':', ',');
    if(conf->tags_s != NULL) free(conf->tags_s);
    conf->tags_s = array_to_container_string(
            conf->tags, conf->tags_n, '[', ']', ',', ',');
}
void free_fields_and_tags(config* conf)
{
    if(conf->fields_s != NULL) free(conf->fields_s);
    if(conf->tags_s != NULL) free(conf->tags_s);
}
int parse_arguments(int argc, char** argv, config* conf)
{
    int i;
    size_t* current_size;
    char* name;
    for(i = 0; i < argc; i++)
    {
        if(strlen(argv[i]) > 2 && argv[i][0] == '-' && argv[i][1] == '-')
        {
            name = argv[i] + 2;
            CONFIG_CURRENT(directories)
            else CONFIG_CURRENT(persist)
            else CONFIG_CURRENT(excluded_files)
            else CONFIG_CURRENT(substitutions)
            else CONFIG_CURRENT(zookeepers)
            else CONFIG_CURRENT(brokers)
            else CONFIG_CURRENT(topic)
            else CONFIG_CURRENT(fields)
            else CONFIG_CURRENT(tags)
            else CONFIG_CURRENT(quota)
            else CONFIG_CURRENT(input)
            else
            {
                printf("unknown option %s\n", argv[i]);
                return 0;
            }
            *current_size = 0;
        }
        else
        {
            (*current_size)++;
        }
    }
    add_fields_and_tags(conf);
    return 1;
}

