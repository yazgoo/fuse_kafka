// LCOV_EXCL_START
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
int tests_run = 0;
void line()
{
    int i;
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    for(i = 0; i < w.ws_col; i++) printf("=");
    printf("\n");
}
int main(int argc, char** argv)
{
    time_t start,end;
    start=clock();
    line();
    char* result = all_tests();
    line();
    if (result != 0) printf("ASSERTION FAILED:\n%s\n", result);
    else printf("ALL TESTS PASSED\n");
    printf("Tests run: %d, duration: %f seconds\n", tests_run,
            (float) (clock()-start)/CLOCKS_PER_SEC);
    line();
    system("rm -f " TEST "/to");
    return result != 0;
}
// LCOV_EXCL_STOP
