/* Fritz 2.5 Reborn — prototipo della schermata principale (Fase 3).
 *
 * Ogni elemento disegnato è una traduzione delle routine di FRITZ2.EXE
 * (indirizzi SEG:OFF dell'eseguibile decompresso).
 *
 * Uso: fritz [--scale N] [--reference cattura.bmp] [--dump out.bmp]
 *   Con --reference: Tab alterna replica/riferimento, D mostra le differenze
 *   (pixel diversi in rosso), Esc esce.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "desktop.h"
#include "gfx.h"
#include "platform.h"
#include "screen.h"

typedef enum View { VIEW_NATIVE, VIEW_REFERENCE, VIEW_DIFF } View;

static void make_diff(const Screen *a, const Screen *b, Screen *out)
{
    for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            out->pixels[y][x] = a->pixels[y][x] == b->pixels[y][x]
                ? (a->pixels[y][x] == COLOR_BLACK ? COLOR_BLACK : COLOR_LIGHT_GRAY)
                : 12; /* rosso chiaro */
}

int main(int argc, char **argv)
{
    static Screen native, reference, diff;
    const char *reference_path = NULL;
    const char *dump_path = NULL;
    int scale = 2;
    View view = VIEW_NATIVE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc)
            scale = atoi(argv[++i]);
        else if (strcmp(argv[i], "--reference") == 0 && i + 1 < argc)
            reference_path = argv[++i];
        else if (strcmp(argv[i], "--dump") == 0 && i + 1 < argc)
            dump_path = argv[++i];
        else {
            fprintf(stderr, "uso: %s [--scale N] [--reference cattura.bmp] [--dump out.bmp]\n", argv[0]);
            return 1;
        }
    }
    if (scale < 1)
        scale = 1;

    if (dump_path) {
        /* Rendering senza finestra: salva il frame ed esce. */
        gfx_set_scale(&gfx_scale_vga);
        screen_clear(&native, COLOR_BLACK);
        desktop_draw(&native);
        return platform_save_screen(&native, dump_path) ? 0 : 1;
    }

    if (!platform_init("Fritz 2.5 Reborn", scale))
        return 1;
    if (reference_path && !platform_load_reference(reference_path, &reference))
        reference_path = NULL;

    gfx_set_scale(&gfx_scale_vga);
    screen_clear(&native, COLOR_BLACK);
    desktop_draw(&native);
    if (reference_path) {
        int different = 0;
        make_diff(&native, &reference, &diff);
        for (int y = 0; y < SCREEN_HEIGHT; y++)
            for (int x = 0; x < SCREEN_WIDTH; x++)
                different += native.pixels[y][x] != reference.pixels[y][x];
        printf("pixel diversi dal riferimento: %d su %d\n",
               different, SCREEN_WIDTH * SCREEN_HEIGHT);
    }

    for (bool running = true; running;) {
        PlatformEvent ev;
        while (platform_poll_event(&ev)) {
            if (ev.type == PLATFORM_EVENT_QUIT)
                running = false;
            else if (ev.type == PLATFORM_EVENT_KEY) {
                if (ev.key == SDLK_ESCAPE)
                    running = false;
                else if (ev.key == SDLK_TAB && reference_path)
                    view = view == VIEW_REFERENCE ? VIEW_NATIVE : VIEW_REFERENCE;
                else if (ev.key == SDLK_D && reference_path)
                    view = view == VIEW_DIFF ? VIEW_NATIVE : VIEW_DIFF;
            }
        }
        platform_present(view == VIEW_REFERENCE ? &reference
                         : view == VIEW_DIFF ? &diff : &native);
        SDL_Delay(16);
    }

    platform_shutdown();
    return 0;
}
