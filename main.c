#include <SDL2/SDL.h>
#include <ctype.h>
#include <curl/curl.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/HTMLparser.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"
#include "net.h"
#include "json.h"

//Storage
struct 
{
    //Chapters
    int chapter_index;
    int chapter_count;
    char** chapter_urls;
} info;

//Helper functions

json_object_entry* json_value_find(json_value* value, char* name)
{
    for(int i = 0; i < value->u.object.length; i++)
    {
        json_object_entry val = value->u.object.values[i];
        if(strcmp(val.name, name) == 0)
            return &value->u.object.values[i];
    }

    return NULL;
}

json_object_entry* json_object_find(json_object_entry* values, size_t len, char* name)
{
    for(int i = 0; i < len; i++)
    {
        if(strcmp(values[i].name, name) == 0)
            return &values[i];
    }

    return NULL;
}

//Checking if xml node is chapter
uint8_t xml_is_chapter(xmlNodePtr node)
{
    xmlAttr* attr;
    for(attr = node->properties; attr; attr = node->properties->next)
    {
        xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
        if(strstr((char*)value, "chapter"))
        {
            xmlFree(value);
            return 1;
        }
        xmlFree(value);
    }
    
    return 0;
}


char* url_safefy(char* input)                                         
{
    //Allocate buffer with same size as initial string
    char* buffer = malloc(strlen(input) * sizeof(char));

    int len = 0;
    for(int i = 0; i < strlen(input); i++)
    {
        //Ignore spaces
        if(isspace(input[i]))
            continue;
        
        //Ignore tabs
        if(input[i] == '	')
            continue;

        //Ignore newlines
        if(input[i] == '\n')
            continue;

        buffer[len++] = input[i];
    }

    //actual buffer
    char* aBuff = malloc(len * sizeof(char));
    memcpy(aBuff, buffer, len * sizeof(char));
    free(buffer);
    return aBuff;
}

float lerp(float v0, float v1, float t) 
{
    return (1 - t) * v0 + t * v1;
}




char* search_for_manga(void)
{
    //Manga name
    char buffer[2048 * sizeof(char)];
    printf("[Manga name]:");
    fgets(buffer, 2048 * sizeof(char), stdin);

    //URLencode name
    char* name = curl_escape(buffer, 0);

    //Preparing POST data
    char* postData = malloc(35 + strlen(name) * sizeof(char));
    sprintf(postData, "action=wp-manga-search-manga&title=%s", name);

    //Posting search info to a server
    webclient_t* client = webclient_init();
    webclient_set_op(client, OP_POST);
    webclient_set_url(client, "https://mangaclash.com/wp-admin/admin-ajax.php");
    webclient_set_post_data(client, postData);
    webclient_perform(client);
    
    //Freeing allocated post data
    free(postData);

    //Parsing response into JSON object
    json_value* json = json_parse(client->response.data, client->response.length);
    json_object_entry* array = json_value_find(json, "data");

    //Checking if successful
    json_object_entry* is_success = json_value_find(json, "success");
    if(!is_success->value->u.boolean)
    {
        printf("No search results!\n");
        return NULL;
    }

    //Allocating memory for URL list
    size_t size = array->value->u.array.length;
    char** urls = malloc(size * sizeof(char*));

    //Filling urls and printing titles
    for(int i = 0; i < array->value->u.array.length; i++)
    {
        size_t len = array->value->u.array.values[i]->u.object.length;
        json_object_entry* values = array->value->u.array.values[i]->u.object.values;
        
        char* name = json_object_find(values, len, "title")->value->u.string.ptr;
        urls[i] = json_object_find(values, len, "url")->value->u.string.ptr;
        printf("[%i] %s\n", i, name);
    }

    //Asking user for manga index
    int index;
    printf("[]:");
    scanf("%i", &index);
    puts("");

    //Bounds check
    if(index < 0) index = 0;
    if(index >= size) index = size - 1;

    //Copying result to new string
    char* url = malloc(strlen(urls[index]) * sizeof(char));
    memcpy(url, urls[index], strlen(urls[index]) * sizeof(char));

    //Freeing memory
    free(urls);
    json_value_free(json);

    return url;
}

void fetch_chapters(char* url)
{
    //Fetching webpage
    webclient_t* client = webclient_init();
    webclient_set_op(client, OP_GET);
    webclient_set_url(client, url);
    webclient_perform(client);

    //Parsing HTML page
    htmlDocPtr doc;
    doc = htmlReadMemory(client->response.data, client->response.length, NULL, NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET);
    
    //Searching for pattern
    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr obj = xmlXPathEval((xmlChar*)"//*[contains(@class, 'wp-manga-chapter  ')]", ctx);
    xmlNodeSetPtr nodeset = obj->nodesetval;
    printf("Found %d chapters:\n", nodeset->nodeNr);

    //Allocating chapter url array
    info.chapter_urls = (char**)malloc(sizeof(char*) * nodeset->nodeNr);
    info.chapter_count = nodeset->nodeNr;

    //Searching for chapters
    for (int i = 0; i < nodeset->nodeNr; i++) 
    {
        xmlNodePtr child;
        for(child = nodeset->nodeTab[i]->children; child; child = child->next)
            if(strcmp((char*)child->name, "a") == 0)
            {
                xmlAttrPtr attr;
                for(attr = child->properties; attr; attr = attr->next)
                {
                    if(strcmp((char*)attr->name, "href") == 0)
                    {
                        xmlChar* value = xmlNodeGetContent(attr->children);
                        info.chapter_urls[nodeset->nodeNr - i - 1] = (char*)value;
                    }
                }

                //Printing chapters in backwards-order (easier to input)
                printf("[%i] %s\n", nodeset->nodeNr - i - 1, strtok((char*)xmlNodeGetContent(child), "\n"));
            }
	}

    //Ask user for input
    printf("[]:");
    scanf("%i", &info.chapter_index);
    puts("");

    //Bounds check
    if(info.chapter_index < 0) info.chapter_index = 0;
    if(info.chapter_index >= nodeset->nodeNr) info.chapter_index = nodeset->nodeNr-1;

    xmlFreeDoc(doc);
}

char** fetch_image_urls(int* arraySize)
{
    char* url = info.chapter_urls[info.chapter_index];

    //Fetching HTML page
    webclient_t* client = webclient_init();
    webclient_set_op(client, OP_GET);
    webclient_set_url(client, url);
    webclient_perform(client);

    //Loading HTML document
    htmlDocPtr doc;
    doc = htmlReadMemory(client->response.data, client->response.length, NULL, NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET);

    //Searching for images on page with wp-manga-chapter-img class
    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr obj = xmlXPathEval((xmlChar*)"//*[contains(@class, 'wp-manga-chapter-img')]", ctx);
    xmlNodeSetPtr nodeset = obj->nodesetval;

    //Allocating url array
    char** urls = (char**)malloc(sizeof(char*) * nodeset->nodeNr);
    *arraySize = nodeset->nodeNr;

    //Checking attrs of html tag
    for (int i = 0; i < *arraySize; i++) 
    {
        xmlNodePtr node = nodeset->nodeTab[i];
        xmlAttrPtr attr;
        for(attr = node->properties; attr; attr = attr->next)
        {
            if(strcmp((char*)attr->name, "data-src") == 0)
            {
                //stupid fix for spaces in url
                urls[i] = url_safefy((char*)xmlNodeGetContent(attr->children));
            }
        }
    }

    return urls;
}

void show_viewer(void)
{
    
    //Init window and renderer
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1, 1, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    //Fetching image urls
    int imagesSize;
    char** imageUrls = fetch_image_urls(&imagesSize);

    //Allocating image array
    image_t* images = malloc(sizeof(image_t) * imagesSize);

    //Load all pages
    for(int i = 0; i < imagesSize; i++)
    {
        printf("Loading images (%i/%i)....\n", i, imagesSize);
        images[i] = image_load(imageUrls[i], renderer);
    }
    //Show window
    SDL_SetWindowSize(window, images[0].w, images[0].h);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    int isRunning = 1;
    int pageY = 0, pageDir = 0;
    
    while(isRunning)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_KEYDOWN)
            {
                switch(e.key.keysym.sym)
                {
                    case SDLK_DOWN:
                        pageDir = -1;
                        break;

                    case SDLK_UP:
                        pageDir = 1;
                        break;

                    case SDLK_LEFT:
                        info.chapter_index--;

                        if(info.chapter_index < 0)
                        {
                            info.chapter_index = 0;
                            break;
                        }

                        //Unload pages
                        for(int i = 0; i < imagesSize; i++)
                            image_dispose(&images[i]);

                        //Fetch new pages
                        imageUrls = fetch_image_urls(&imagesSize);

                        //Load all pages
                        for(int i = 0; i < imagesSize; i++)
                        {
                            printf("Loading images (%i/%i)....\n", i, imagesSize);
                            images[i] = image_load(imageUrls[i], renderer);
                        }
                        pageY = 0;
                        break;
                    
                    case SDLK_RIGHT:
                        info.chapter_index++;

                        if(info.chapter_index >= info.chapter_count)
                        {
                            info.chapter_index = info.chapter_count-1;
                            break;
                        }

                        //Unload pages
                        for(int i = 0; i < imagesSize; i++)
                            image_dispose(&images[i]);

                        //Fetch new pages
                        imageUrls = fetch_image_urls(&imagesSize);

                        //Load all pages
                        for(int i = 0; i < imagesSize; i++)
                        {
                            printf("Loading images (%i/%i)....\n", i, imagesSize);
                            images[i] = image_load(imageUrls[i], renderer);
                        }
                        pageY = 0;
                        break;
                }
            }
            else if(e.type == SDL_KEYUP)
                pageDir = 0;
            else if(e.type == SDL_QUIT)
                isRunning = false;
        }

        SDL_RenderClear(renderer);

        int wX, wY, yy = 0;
        SDL_GetWindowSize(window, &wX, &wY);
        for(int i = 0; i < imagesSize; i++)
        {
            image_t img = images[i];
            SDL_Rect rect = {.x = wX / 2 - img.w / 2, .y = yy - pageY, .w = img.w, .h = img.h};
            SDL_RenderCopy(renderer, img.texture, NULL, &rect);
            yy += img.h;
        }

        if(pageY < 0) pageY = 0;
        if(pageY > yy - images[imagesSize-1].h) pageY = yy - images[imagesSize-1].h;

        SDL_RenderPresent(renderer);
        pageY -= pageDir * 10;
        SDL_Delay(15);
    }
}

int main(void)
{
    char* mangaUrl = search_for_manga();

    //Restart if manga not found
    if(mangaUrl == NULL)
        main();

    fetch_chapters(mangaUrl);
    show_viewer();
    return 0;
}