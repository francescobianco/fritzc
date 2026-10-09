#include "platform.h"

#include <SDL3/SDL.h>

#include "palette.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

bool platform_init(const char *title, int scale)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return false;
    }
    if (!SDL_CreateWindowAndRenderer(title, SCREEN_WIDTH * scale,
                                     SCREEN_HEIGHT * scale,
                                     SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer: %s", SDL_GetError());
        return false;
    }
    /* Integer scaling with nearest neighbour keeps the original pixels square and sharp. */
    SDL_SetRenderLogicalPresentation(renderer, SCREEN_WIDTH, SCREEN_HEIGHT,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!texture) {
        SDL_Log("SDL_CreateTexture: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_HideCursor();   /* the program draws its own pointer */
    return true;
}

void platform_shutdown(void)
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool platform_poll_event(PlatformEvent *event)
{
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        SDL_ConvertEventToRenderCoordinates(renderer, &e);
        *event = (PlatformEvent){0};
        switch (e.type) {
        case SDL_EVENT_QUIT:
            event->type = PLATFORM_EVENT_QUIT;
            return true;
        case SDL_EVENT_KEY_DOWN:
            event->type = PLATFORM_EVENT_KEY;
            event->key = (int)e.key.key;
            event->down = true;
            return true;
        case SDL_EVENT_MOUSE_MOTION:
            event->type = PLATFORM_EVENT_MOUSE_MOVE;
            event->x = (int)e.motion.x;
            event->y = (int)e.motion.y;
            return true;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            event->type = PLATFORM_EVENT_MOUSE_BUTTON;
            event->x = (int)e.button.x;
            event->y = (int)e.button.y;
            event->button = e.button.button;
            event->down = e.button.down;
            return true;
        default:
            break;
        }
    }
    return false;
}

void platform_present(const Screen *screen)
{
    void *pixels;
    int pitch;

    if (SDL_LockTexture(texture, NULL, &pixels, &pitch)) {
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            Uint32 *row = (Uint32 *)((Uint8 *)pixels + y * pitch);
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                Rgb c = palette_vga16[screen->pixels[y][x] & 15];
                row[x] = (Uint32)c.r << 16 | (Uint32)c.g << 8 | c.b;
            }
        }
        SDL_UnlockTexture(texture);
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

bool platform_load_reference(const char *path, Screen *out)
{
    SDL_Surface *bmp = SDL_LoadBMP(path);
    bool ok = false;

    if (!bmp) {
        SDL_Log("SDL_LoadBMP %s: %s", path, SDL_GetError());
        return false;
    }
    if (bmp->w == SCREEN_WIDTH && bmp->h == SCREEN_HEIGHT &&
        bmp->format == SDL_PIXELFORMAT_INDEX8) {
        for (int y = 0; y < SCREEN_HEIGHT; y++)
            SDL_memcpy(out->pixels[y], (Uint8 *)bmp->pixels + y * bmp->pitch,
                       SCREEN_WIDTH);
        ok = true;
    } else {
        SDL_Log("%s: an 8-bit indexed 640x480 BMP is required", path);
    }
    SDL_DestroySurface(bmp);
    return ok;
}

bool platform_save_screen(const Screen *screen, const char *path)
{
    SDL_Surface *surface = SDL_CreateSurface(SCREEN_WIDTH, SCREEN_HEIGHT,
                                             SDL_PIXELFORMAT_INDEX8);
    SDL_Palette *palette;
    bool ok = false;

    if (!surface)
        return false;
    palette = SDL_CreateSurfacePalette(surface);
    if (palette) {
        SDL_Color colors[16];
        for (int i = 0; i < 16; i++)
            colors[i] = (SDL_Color){palette_vga16[i].r, palette_vga16[i].g,
                                    palette_vga16[i].b, 255};
        SDL_SetPaletteColors(palette, colors, 0, 16);
        for (int y = 0; y < SCREEN_HEIGHT; y++)
            SDL_memcpy((Uint8 *)surface->pixels + y * surface->pitch,
                       screen->pixels[y], SCREEN_WIDTH);
        ok = SDL_SaveBMP(surface, path);
    }
    SDL_DestroySurface(surface);
    return ok;
}
