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

/* Event already converted to virtual-surface coordinates. */
typedef struct PlatformEvent {
    PlatformEventType type;
    int key;      /* SDL_Keycode for PLATFORM_EVENT_KEY */
    int x, y;     /* virtual coordinates for mouse events */
    int button;
    bool down;
} PlatformEvent;

bool platform_init(const char *title, int scale);
void platform_shutdown(void);
bool platform_poll_event(PlatformEvent *event);
void platform_present(const Screen *screen);

/* Loads a reference screenshot (8-bit BMP, 640x480, VGA palette). */
bool platform_load_reference(const char *path, Screen *out);

/* Saves the surface as an 8-bit indexed BMP (for comparison tests). */
bool platform_save_screen(const Screen *screen, const char *path);

#endif
