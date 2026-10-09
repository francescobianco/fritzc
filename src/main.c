/* Fritz 2.5 Reborn — main screen prototype (phase 3).
 *
 * Everything drawn here is a translation of FRITZ2.EXE routines
 * (SEG:OFF addresses in the unpacked executable). Artwork and texts are read
 * at runtime from the user's original distribution (--data).
 *
 * Usage: fritz [--data DIR] [--par FRITZ2.PAR] [--free-kb N] [--scale N]
 *              [--reference capture.bmp] [--dump out.bmp]
 *   --data DIR   original Fritz 2.51 files (default: dos/fritz251)
 *   --par FILE   settings file (default: DIR/FRITZ2.PAR, if present)
 *   --free-kb N  free memory shown in the status panel (default 105, what
 *                the original FRITZ2.EXE shows in the reference DOSBox)
 *   With --reference: Tab toggles port/reference, D shows the differences
 *   (mismatching pixels in red), Esc quits.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "desktop.h"
#include "exe.h"
#include "font.h"
#include "game_board.h"
#include "gfx.h"
#include "language.h"
#include "logo.h"
#include "platform.h"
#include "screen.h"
#include "settings.h"
#include "sprites.h"
#include "ui_state.h"

typedef enum View { VIEW_NATIVE, VIEW_REFERENCE, VIEW_DIFF } View;

static void make_diff(const Screen *a, const Screen *b, Screen *out)
{
    for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
            out->pixels[y][x] = a->pixels[y][x] == b->pixels[y][x]
                ? (a->pixels[y][x] == COLOR_BLACK ? COLOR_BLACK : COLOR_LIGHT_GRAY)
                : 12; /* light red */
}

static const char *data_file(const char *dir, const char *name)
{
    static char path[1024];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    return path;
}

static void render(Screen *screen, const Exe *exe, const Settings *settings,
                   const Language *lan, const Logo *logo, long free_kb)
{
    GameBoard board;
    UiState ui;

    ui_state_init(&ui, exe, settings);
    if (free_kb >= 0)
        ui.free_memory_kb = free_kb;

    /* Start position: FRITZ2.PAR game record (0000:1483 copies DS:AEF6),
       or a new game (0a1f:70ed). */
    if (settings->has_game)
        memcpy(board.squares, settings->start_board, sizeof board.squares);
    else
        game_board_setup(&board);
    gfx_set_scale(&gfx_scale_vga);
    screen_clear(screen, COLOR_BLACK);
    desktop_draw(screen, settings, lan, &board, logo, &ui);
}

int main(int argc, char **argv)
{
    static Screen native, reference, diff;
    const char *data_dir = "dos/fritz251";
    const char *reference_path = NULL;
    const char *dump_path = NULL;
    const char *par_path = NULL;
    char error[256];
    Settings settings;
    Language lan;
    Logo logo;
    Exe exe;
    int scale = 2;
    long free_kb = -1;
    View view = VIEW_NATIVE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc)
            scale = atoi(argv[++i]);
        else if (strcmp(argv[i], "--free-kb") == 0 && i + 1 < argc)
            free_kb = atol(argv[++i]);
        else if (strcmp(argv[i], "--data") == 0 && i + 1 < argc)
            data_dir = argv[++i];
        else if (strcmp(argv[i], "--reference") == 0 && i + 1 < argc)
            reference_path = argv[++i];
        else if (strcmp(argv[i], "--par") == 0 && i + 1 < argc)
            par_path = argv[++i];
        else if (strcmp(argv[i], "--dump") == 0 && i + 1 < argc)
            dump_path = argv[++i];
        else {
            fprintf(stderr, "usage: %s [--data DIR] [--par FRITZ2.PAR] [--free-kb N] [--scale N] "
                            "[--reference capture.bmp] [--dump out.bmp]\n", argv[0]);
            return 1;
        }
    }
    if (scale < 1)
        scale = 1;

    if (exe_load(&exe, data_file(data_dir, "FRITZ2.EXE"), error, sizeof error) != 0) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    if (!fonts_load(&exe) || !sprites_load(&exe)) {
        fprintf(stderr, "FRITZ2.EXE: font or bitmap data not found\n");
        return 1;
    }

    settings_defaults(&settings);
    if (!par_path)
        par_path = data_file(data_dir, "FRITZ2.PAR");
    if (!settings_load_par(&settings, par_path))
        fprintf(stderr, "%s: not a valid FRITZ2.PAR, using defaults\n", par_path);
    if (!language_load(&lan, data_file(data_dir, settings.language_file)))
        fprintf(stderr, "%s: cannot load the language file\n", settings.language_file);
    if (!logo_load(&logo, data_file(data_dir, "FRITZ2.IMG")))
        fprintf(stderr, "FRITZ2.IMG: cannot load the logo\n");

    if (dump_path) {
        /* Headless rendering: save the frame and exit. */
        render(&native, &exe, &settings, &lan, &logo, free_kb);
        return platform_save_screen(&native, dump_path) ? 0 : 1;
    }

    if (!platform_init("Fritz 2.5 Reborn", scale))
        return 1;
    if (reference_path && !platform_load_reference(reference_path, &reference))
        reference_path = NULL;

    render(&native, &exe, &settings, &lan, &logo, free_kb);
    if (reference_path) {
        int different = 0;
        make_diff(&native, &reference, &diff);
        for (int y = 0; y < SCREEN_HEIGHT; y++)
            for (int x = 0; x < SCREEN_WIDTH; x++)
                different += native.pixels[y][x] != reference.pixels[y][x];
        printf("pixels differing from reference: %d of %d\n",
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
    language_free(&lan);
    logo_free(&logo);
    exe_free(&exe);
    return 0;
}
