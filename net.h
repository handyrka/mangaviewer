#ifndef H_NET
#define H_NET
#include <curl/curl.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    char* url;
    CURL* curl;
    CURLcode code;

    struct 
    {
        char* data;
        size_t length;
    } response;

} webclient_t;

#define OP_GET 0
#define OP_POST 1

webclient_t* webclient_init();
void webclient_disopse(webclient_t* client);
void webclient_set_url(webclient_t* client, const char* url);
void webclient_set_op(webclient_t* client, const char op);
void webclient_set_post_data(webclient_t* client, char* data);
void webclient_set_post_type(webclient_t* client, const char* type);
void webclient_perform(webclient_t* client);

#endif