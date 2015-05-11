/** @file 
 * @brief utility functions
 **/ 
#ifndef UTIL_C
#define UTIL_C
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <stdlib.h>
#include <string.h>
int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1==*s2))
        s1++,s2++;
    return *(const unsigned char*)s1-*(const unsigned char*)s2;
}
/**
 * @brief convert the input to base64
 * @param input the input string
 * @param size of the input string
 * @return a newly allocated string with the base64 data which should
 * be free'd by the user
 **/
char *base64(const unsigned char *input, int length)
{
    BIO *bmem, *b64;
    BUF_MEM *bptr;
    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);
    char *buff = (char *)malloc(bptr->length + 1);
    memcpy(buff, bptr->data, bptr->length);
    buff[bptr->length] = 0;
    BIO_free_all(b64);
    return buff;
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
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    strftime(timestamp, strlen(timestamp), "%Y-%m-%dT%H:%M:%S.000%z", &tm);
}
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
char* concat(char* a, char* b)
{
    char* result = (char*) malloc(strlen(a) + strlen(b) + 2);
    char* middle = result + strlen(a);
    strcpy(result, a);
    middle[0] = '/';
    strcpy(++middle, b);
    return result;
}
#define DO_AS_CALLER(action) \
    struct fuse_context* __context = fuse_get_context(); \
    gid_t __gid = getegid(); \
    uid_t __uid = geteuid(); \
    seteuid(__context->uid); \
    setegid(__context->gid); \
    action \
    setegid(__gid); \
    seteuid(__uid);
#endif
