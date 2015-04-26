#include <input_plugin.h>
#include <unistd.h>
int input_setup(int argc, char** argv, void* conf)
{
    do output_write("example", "ping", strlen("ping"), 0);
    while(sleep(1) >= 0);
}
