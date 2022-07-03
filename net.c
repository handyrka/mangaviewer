#include "net.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/header.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t webclient_write(void* data, size_t size, size_t nmemb, webclient_t* client)
{
    size_t realsize = size * nmemb;

    char *ptr = realloc(client->response.data, client->response.length + realsize + 1);
    if(ptr == NULL)
      return 0;  /* out of memory! */
    
    client->response.data = ptr;
    memcpy(&(client->response.data[client->response.length]), data, realsize);
    client->response.length += realsize;
    client->response.data[client->response.length] = 0;
    
    return realsize;
}

webclient_t* webclient_init()
{
    webclient_t* client = (webclient_t*) malloc(sizeof(webclient_t)); 
    client->curl = curl_easy_init();
    client->response.data = NULL;
    client->response.length = 0;

    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, webclient_write);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, client);
    //curl_easy_setopt(client->curl, CURLOPT_VERBOSE, 1L);
    
    return client;
}

void webclient_disopse(webclient_t* client)
{
    free(client->response.data); //freeing memory
    curl_easy_cleanup(client->curl);
    free(client);
    client = NULL; //destroying pointer
}

void webclient_set_url(webclient_t *client, const char *url)
{
    client->url = (char*)url;
    curl_easy_setopt(client->curl, CURLOPT_URL, url);
}

void webclient_set_op(webclient_t *client, const char op)
{
    curl_easy_setopt(client->curl, CURLOPT_POST, op);
}

void webclient_set_post_data(webclient_t *client, char *data)
{
    curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, data);
}

void webclient_set_post_type(webclient_t *client, const char *type)
{
    char* content_type = "Content-Type: ";
    size_t len = strlen(content_type);
    char* buffer = (char*) malloc(len + strlen(type) + 1);
    sprintf(buffer, "%s%s", content_type, type);

    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, buffer);
    curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, chunk); 
}

void webclient_perform(webclient_t *client)
{
    if(client->response.data != NULL)
        free(client->response.data);
    
    client->response.data = (char*)malloc(1);
    client->code = curl_easy_perform(client->curl);
}