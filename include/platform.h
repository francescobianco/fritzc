#ifndef FRITZ_PLATFORM_H
#define FRITZ_PLATFORM_H

#include <stdbool.h>

#include "screen.h"

typedef enum PlatformEventType {
    PLATFORM_EVENT_NONE,
    PLATFORM_EVENT_QUIT,
    PLATFORM_EVENT_KEY,
    PLATFORM_EVENT_MOUSE_MOVE,
    PLATFORM_EVENT_MOUSE_BUTTON
} PlatformEventType;

/* Evento già convertito in coordinate della superficie virtuale. */
typedef struct PlatformEvent {
    PlatformEventType type;
    int key;      /* SDL_Keycode per PLATFORM_EVENT_KEY */
    int x, y;     /* coordinate virtuali per gli eventi del mouse */
    int button;
    bool down;
} PlatformEvent;

bool platform_init(const char *title, int scale);
void platform_shutdown(void);
bool platform_poll_event(PlatformEvent *event);
void platform_present(const Screen *screen);

/* Carica uno screenshot di riferimento (BMP 8 bit, 640x480, palette VGA). */
bool platform_load_reference(const char *path, Screen *out);

/* Salva la superficie come BMP indicizzato 8 bit (per i test di confronto). */
bool platform_save_screen(const Screen *screen, const char *path);

#endif
