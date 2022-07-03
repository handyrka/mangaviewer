#ifndef H_IMAGE
#define H_IMAGE

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <stddef.h>
#include <stdint.h>

typedef struct 
{
    SDL_Texture* texture;
    SDL_Surface* surface;

    struct
    {
        char* buffer;
        size_t length;
    } data;

    int32_t w, h;
} image_t;

image_t image_load(const char* url, SDL_Renderer* renderer);
void image_dispose(image_t* image);

#endif