#include <input_plugin.h>
#include <unistd.h>
int input_setup(int argc, char** argv, void* conf)
{
    fuse_get_context()->private_data = conf;
    fuse_get_context()->private_data = output_init((config*) conf);
    char* line = "ping";
    while(1)
    {
        printf("%s\n", line);
        if(should_write_to_kafka("example", strlen(line)))
            actual_kafka_write("example", line, strlen(line), 0);
        sleep(1);
    }
}
