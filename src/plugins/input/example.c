#include <input_plugin.h>
#include <unistd.h>
#include <stdio.h>
int input_setup(int argc, char** argv, void* conf)
{
    do output_write("example", "ping", strlen("ping"), 0);
    while(fk_sleep(1) >= 0);
}
