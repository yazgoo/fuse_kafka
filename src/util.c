/** @file 
 * @brief utility functions
 **/ 
#ifndef UTIL_C
#define UTIL_C
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1==*s2))
        s1++,s2++;
    return *(const unsigned char*)s1-*(const unsigned char*)s2;
}
int touch(char* path, char* str)
{
    FILE* f = fopen(path, "w");
    if(f == NULL) return 0;
    fwrite(str, strlen(str), 1, f);
    fclose(f);
    return 1;
}
char* integer_concat(char* prefix, int i, char* suffix)
{
    int size = (strlen(prefix) + ceil(log10(i))+1 + strlen(suffix))*sizeof(char);
    char *str = (char*) malloc((int)(size) + 1);
    if(str != NULL)
        snprintf(str, size, "%s%d%s", prefix, i, suffix);
    return str;
}
#define BASE64_CHARS \
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
/**
 * @brief convert the input to base64
 * @param input the input string
 * @param size of the input string
 * @return a newly allocated string with the base64 data which should
 * be free'd by the user
 **/
char* base64(const unsigned char* str, int n)
{
    static const char* base64chars = BASE64_CHARS;
    if(str == NULL) return NULL;
    int i = 0, j, k;
    int added = n % 3;
    added = (added == 0? 0: (added == 1? 2 : 1));
    int size = (n + added) * 8 / 6;
    char* result = (char*) calloc(sizeof(char), size + added + 1);
    result[size] = 0;
    for(j = 0; j < (n + added); j++)
    {
        char c = j < size ? str[j] : 0;
        for(k = 0; k < 8; k++)
        {
            int i_k = i + k;
            int i_k_6 = i_k / 6;
            if((i_k % 6) == 0)
            {
                if(i_k_6 >= 1) result[i_k_6 - 1] = base64chars[result[i_k_6 - 1]];
                result[i_k_6] = 0;
            }
            result[i_k_6] = (result[i_k_6] << 1) | ((c >> (7 - k)) & 1);
        }
        i += 8;
    }
    /* for last char, base64chars might not have been called: */
    if(added == 0 && size > (added + 1)) result[size - 1 - added] = base64chars[result[size - 1 - added]];
    for(j = 0; j < added; j++) result[size - 1 - j] = '=';
    return result;
}
int* get_command_line_size()
{
    static int command_line_size = 256;
    return &command_line_size;
}
/**
 * @brief get the command line corresponding to a process
 * @param pid process id
 * @return a base64 encoded string with the process command line, or
 * an empty string if it could not be extrcated
 **/
static char* get_command_line(int pid)
{
    size_t size = *get_command_line_size();;
    size_t i = 0;
    char* path;
    FILE* f;
    char c;
    char* command_line = (char*) malloc(size * sizeof(char));
    char* b64;
    asprintf(&path, "/proc/%d/cmdline", pid);
    if((f = fopen(path, "r")) != NULL)
    {
        while((c = fgetc(f)) != EOF)
        {
            if(c == 0) c = ' ';
            if(i >= (size - 1))
            {
                size += *get_command_line_size();
                command_line = (char*) realloc(
                        command_line, size * sizeof(char));
            }
            command_line[i++] = c;
        }
        fclose(f);
    }
    command_line[i] = 0;
    free(path);
    b64 = base64(command_line, strlen(command_line));
    free(command_line);
    return b64;
}
/**
 * @brief get a string representing the time in ISO8601 format
 * @param timestamp string to set to the given time
 * @return void
 **/
void set_timestamp(char* timestamp)
{
    struct timeval tv;
    struct tm tm;
    struct tm* tmp;
    gettimeofday(&tv, NULL);
#ifdef MINGW_VER
    tmp = localtime(&tv.tv_sec);
#else
    localtime_r(&tv.tv_sec, &tm);
    tmp = &tm;
#endif
    strftime(timestamp, strlen(timestamp), "%Y-%m-%dT%H:%M:%S.000%z", tmp);
}
#ifdef MINGW_VER
/* TODO implement */
int fnmatch(char* pattern, char* string, int flags)
{
    return 0;
}
int flock(int fd, int operation)
{
    return 0;
}
int fchdir(int fd)
{
    return 0;
}
#include <errno.h>
/* implementation from 
http://stackoverflow.com/questions/27369580/codeblocks-and-c-undefined-reference-to-getline
*/
ssize_t getdelim(char **linep, size_t *n, int delim, FILE *fp){
    int ch;
    size_t i = 0;
    if(!linep || !n || !fp){
        errno = EINVAL;
        return -1;
    }
    if(*linep == NULL){
        if(NULL==(*linep = malloc(*n=128))){
            *n = 0;
            errno = ENOMEM;
            return -1;
        }
    }
    while((ch = fgetc(fp)) != EOF){
        if(i + 1 >= *n){
            char *temp = realloc(*linep, *n + 128);
            if(!temp){
                errno = ENOMEM;
                return -1;
            }
            *n += 128;
            *linep = temp;
        }
        (*linep)[i++] = ch;
        if(ch == delim)
            break;
    }
    (*linep)[i] = '\0';
    return !i && ch == EOF ? -1 : i;
}
ssize_t getline(char **linep, size_t *n, FILE *fp){
    return getdelim(linep, n, '\n', fp);
}
#endif
/**
 * @brief look for the limit "--" in argv
 * @argc argument number
 * @argv arguments
 * @return the index of the limit, argc if it is not found
 */
int get_limit(int argc, char** argv)
{
    int i = 0;
    for(; i < argc; i++) if(!strcmp(argv[i], "--")) break;
    return i;
}
/**
 * @brief get a concatenated string from an array of string
 */
char* array_to_container_string(char** array, size_t n, char open_char,
        char close_char, char sep1, char sep2)
{
    int i = 0;
    char* str = (char*) malloc(3);
    int k = sprintf(str, "%c", open_char);
    if(array != NULL)
        for(i = 0; i < n; i++)
        {
            str = realloc(str, k + 1 + strlen(array[i]) + 2 + 2);
            k += sprintf(str + k, "\"%s\"", array[i]);
            if(i != n-1) k += sprintf(str + k, "%c ", i % 2 ? sep2 : sep1);
        }
    sprintf(str + k, "%c", close_char);
    return str;
}
int* falloc_fails()
{
    static int result = 0;
    return &result;
}
int* fcalloc_fails()
{
    static int result = 0;
    return &result;
}
void* fmalloc(size_t size)
{
    if(*falloc_fails()) return NULL;
    return malloc(size);
}
void* fcalloc(size_t nmemb, size_t size)
{
    if(*falloc_fails() || *fcalloc_fails()) return NULL;
    return calloc(nmemb, size);
}
void* frealloc(void* ptr, size_t size)
{
    if(*falloc_fails()) return NULL;
    return realloc(ptr, size);
}
/**
 * @return null if a or b is null, a + "/" + b otherwise
 */
char* concat(char* a, char* b)
{
    if(a == NULL || b == NULL) return NULL;
    int length = strlen(a) + strlen(b) + 2;
    char* result = (char*) malloc(length);
    result[length - 1] = 0;
    char* middle = result + strlen(a);
    strcpy(result, a);
    middle[0] = '/';
    strcpy(++middle, b);
    return result;
}
int* fk_sleep_return_value()
{
    static int value = 0;
    return &value;
}
int* fk_sleep_enabled()
{
    static int enabled = 1;
    return &enabled;
}
int fk_sleep(int n)
{
    if(*(fk_sleep_enabled())) return sleep(n);
    return *(fk_sleep_return_value());
}
#ifdef _WIN32
#define mkdir( D, M ) _mkdir( D )
#endif 
static void mkdir_p(const char *dir) {
        char tmp[256];
        char *p = NULL;
        size_t len;

        snprintf(tmp, sizeof(tmp),"%s",dir);
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
                if(*p == '/') {
                        *p = 0;
                        mkdir(tmp, S_IRWXU);
                        *p = '/';
                }
        mkdir(tmp, S_IRWXU);
}
#define DO_AS_CALLER(action) \
    struct fuse_context* __context = fuse_get_context(); \
    gid_t __gid = getegid(); \
    uid_t __uid = geteuid(); \
    setegid(__context->gid); \
    seteuid(__context->uid); \
    action \
    seteuid(__uid); \
    setegid(__gid);
#endif
