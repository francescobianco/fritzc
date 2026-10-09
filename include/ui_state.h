#ifndef FRITZ_UI_STATE_H
#define FRITZ_UI_STATE_H

#include <stdint.h>

#include "exe.h"
#include "settings.h"

/* Program state flags read by the screen routines. Values are those in
   effect when the start screen is drawn. */
typedef struct UiState {
    uint8_t engine_active;      /* [005E]: toggled by 0000:4c4c, on at start */
    uint8_t side_flag;          /* [ADB3] (from [B14A]): swaps clock colours */
    uint8_t hide_marker[2];     /* [0060], [0062]: set to 1 by 0000:81a5 */
    uint8_t message_busy;       /* [ADA4] | [0050] */
    uint8_t show_opening;       /* [AE6A] */
    char book_dir[70];          /* DS:42BE, from FRITZ2.PAR 0x13A */
    char book_name[14];         /* DS:4304, from FRITZ2.PAR 0x180 */
    char book_line_left[82];    /* DS:42A8: empty at start */
    char clock_text[2][9];      /* clock widgets DS:6EF4, DS:6F1B */
    int16_t game_number;        /* [ADD4]: 0 = no database game loaded */
    int16_t ply;                /* [B156]: set to 1 by 0000:1483 */
    int8_t move_parity;         /* [B160] */
    int16_t move_offset;        /* [AD7A] */
    int16_t input_column;       /* [006C]: length of the move prefix */
    /* Shown by the main loop 0000:7a39: largest free DOS block (int 21h/48h,
       0a1f:005c) in kB. The port has no DOS memory, so it shows the value
       the original prints in the reference DOSBox setup. */
    long free_memory_kb;
    int hash_exponent;          /* engine p[0xFE], -1 if not started */
} UiState;

/* Start-screen state; clock strings come from the widgets' initial data. */
void ui_state_init(UiState *ui, const Exe *exe, const Settings *settings);

#endif
