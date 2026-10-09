#ifndef FRITZ_DESKTOP_H
#define FRITZ_DESKTOP_H

#include "game_board.h"
#include "language.h"
#include "logo.h"
#include "screen.h"
#include "settings.h"

void desktop_draw(Screen *screen, const Settings *settings, const Language *lan,
                  const GameBoard *board, const Logo *logo);

#endif
