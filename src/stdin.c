#include <input_plugin.h>
#include <stdio.h>
int input_setup(int argc, char** argv, void* conf)
{
    char line[1024];
    while(fgets(line, 1024, stdin))
        actual_kafka_write("stdin", line, strlen(line) + 1, 0);
}
