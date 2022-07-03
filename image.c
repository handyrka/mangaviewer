#include "image.h"
#include "net.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <string.h>

image_t image_load(const char* url, SDL_Renderer* renderer)
{
    image_t img = {};
    webclient_t* c = webclient_init();
    webclient_set_url(c, url);
    webclient_set_op(c, OP_GET);
    webclient_perform(c);
    img.data.buffer = (char*)malloc(c->response.length);
    img.data.length = c->response.length;
    memcpy(img.data.buffer, c->response.data, c->response.length);
    webclient_disopse(c);

    //load image from memory now
    SDL_RWops *rw = SDL_RWFromMem(img.data.buffer, img.data.length);
    img.surface = IMG_Load_RW(rw, 1);
    img.texture = SDL_CreateTextureFromSurface(renderer, img.surface);
    SDL_QueryTexture(img.texture, NULL, NULL, &img.w, &img.h);
    return img;
}

void image_dispose(image_t* image)
{
    free(image->data.buffer);
    SDL_FreeSurface(image->surface);
    SDL_DestroyTexture(image->texture);
}