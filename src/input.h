#ifndef INPUT_H
#define INPUT_H
int input_setup(int argc, char** argv, void* conf);
typedef int (*input_setup_t)(int argc, char** argv, void* conf);
#endif
