#ifndef CONFIG_H
#define CONFIG_H
#define CONFIG_ITEM(name) char** name; size_t name ## _n;
#include "time_queue.h"
/**
 * @brief fuse_kafka configuration
 **/
typedef struct _config {
    /** @brief file descriptor to the directory under the mount */
    int directory_fd;
    /** @brief number of the directory to mount amongst the directories list */
    size_t directory_n;
    /** @brief string containing a json like hash of string with
     * the fields provided for each event */
    char* fields_s;
    /** @brief string containing a json like array of string with tags
     * for each event */
    char* tags_s;
    /** @brief time queue (@see time_queue) used 
     * in case of quota management */
    time_queue* quota_queue;
    /** @brief directories amongst which the mounted directory is */
    CONFIG_ITEM(directories)
    /** @brief TODO not implemented: do actually overlay files actions
     * to the disk */ 
    CONFIG_ITEM(persist)
    /** @brief files fnmatch based pattern we don't want saved to kafka */
    CONFIG_ITEM(excluded_files)
    /** @brief TODO not implented: substitutions to do to on the command
     * lines */
    CONFIG_ITEM(substitutions)
    /** @brief zookeepers pointing to kafka brokers to write to */ 
    CONFIG_ITEM(zookeepers)
    /** @brief kafka brokers to write to */ 
    CONFIG_ITEM(brokers)
    /** @brief kafka topic to write events to */ 
    CONFIG_ITEM(topic)
    /** @brief logstash fields to add to each event */
    CONFIG_ITEM(fields)
    /** @brief logstash tags */
    CONFIG_ITEM(tags)
    /** @brief arguments being quota and optionnaly size of the quota
     * queue, default being 20; if those arguments are given, if the
     * defined quota */
    CONFIG_ITEM(quota)
    /** @brief input plugin */
    CONFIG_ITEM(input)
} config;
#endif
