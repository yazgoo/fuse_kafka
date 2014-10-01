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
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);
    char *buff = (char *)malloc(bptr->length);
    memcpy(buff, bptr->data, bptr->length-1);
    buff[bptr->length-1] = 0;
    BIO_free_all(b64);
    return buff;
}
static char* get_command_line(unsigned int pid)
{
    char* path, *out;
    char* command_line = NULL;
    size_t size = 0;
    size_t i;
    FILE* f = NULL;
    asprintf(&path, "/proc/%d/cmdline", pid);
    f = fopen(path, "r");
    free(path);
    if(f == NULL || getline(&command_line, &size, f) == -1)
    {
        out = (char*) malloc(1); out[0] = 0;
        return out;
    }
    else
    {
        printf("%d\n", (int) size);
        fclose(f);
        for(i = 0; i < size-1; i++)
            if(!command_line[i] && command_line[i+1]) command_line[i] = ' ';
        out = base64(command_line, size);
        free(command_line);
        return out;
    }
}
void set_timestamp(char* timestamp)
{
    struct timeval tv;
    struct tm tm;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    strftime(timestamp, strlen(timestamp), "%Y-%m-%dT%H:%M:%S.000%z", &tm);
}
