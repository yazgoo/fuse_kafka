#ifndef INPUT_H
#define INPUT_H
#include "config.h"
#include "util.c"
#ifndef TEST
#include "context.c"
#include "output.c"
#else
#include "input_plugin_test.c"
#endif /* TEST */
#include "input.h"
#define requires(lol) /* lol */
#endif /* INPUT_H */
