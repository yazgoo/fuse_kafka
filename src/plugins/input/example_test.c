// LCOV_EXCL_START
#include "minunit.h"
#include "example.c"
static char* test_other_functions()
{
    char* argv[] = {}; 
    *(fk_sleep_enabled()) = 0;
    *(fk_sleep_return_value()) = -1;
    mu_assert("input_setup should return 1", input_setup(0, NULL, NULL) == 1);
    input_is_watching_directory(NULL);
    *(falloc_fails()) = 1;
    input_is_watching_directory(NULL);
    *(falloc_fails()) = 0;
    return 0;
}
static char* all_tests()
{
    mu_run_test(test_other_functions);
    return 0;
}
#include "minunit.c"
// LCOV_EXCL_STOP
