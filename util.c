#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <stdlib.h>
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
    char *buff = (char *)malloc(bptr->length);
    memcpy(buff, bptr->data, bptr->length-1);
    buff[bptr->length-1] = 0;
    BIO_free_all(b64);
    return buff;
}
static char* get_command_line(int pid)
{
    size_t size = 256;
    size_t i = 0;
    char* path;
    FILE* f;
    char c;
    char* string = (char*) malloc(size);
    char* b64;
    asprintf(&path, "/proc/%d/cmdline", pid);
    printf("%s\n", path);
    if((f = fopen(path, "r")) != NULL)
    {
        while((c = fgetc(f)) != EOF)
        {
            if(c == 0) c = ' ';
            if(i >= (size - 1))
            {
                size += 256;
                char* string = (char*) realloc(string, size);
            }
            string[i++] = c;
        }
        fclose(f);
    }
    string[i] = 0;
    free(path);
    b64 = base64(string, strlen(string));
    free(string);
    return b64;
}
void set_timestamp(char* timestamp)
{
    struct timeval tv;
    struct tm tm;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    strftime(timestamp, strlen(timestamp), "%Y-%m-%dT%H:%M:%S.000%z", &tm);
}
