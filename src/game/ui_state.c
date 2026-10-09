#include "ui_state.h"

#include <string.h>

static void copy_string(char *dst, size_t size, const char *src)
{
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

void ui_state_init(UiState *ui, const Exe *exe, const Settings *settings)
{
    static const uint16_t clock_widgets[2] = {0x6ef4, 0x6f1b};

    memset(ui, 0, sizeof *ui);
    ui->engine_active = 1;
    ui->side_flag = settings->side_flag;
    ui->show_opening = settings->show_opening;
    ui->ply = 1;
    ui->move_parity = settings->move_parity;
    ui->move_offset = settings->move_offset;
    ui->hide_marker[0] = 1;
    ui->hide_marker[1] = 1;
    ui->message_busy = 0;
    ui->game_number = 0;
    ui->free_memory_kb = 105;
    copy_string(ui->book_dir, sizeof ui->book_dir, settings->book_dir);
    copy_string(ui->book_name, sizeof ui->book_name, settings->book_name);
    for (int i = 0; i < 2; i++) {
        const uint8_t *text = exe_at(exe, EXE_DGROUP, clock_widgets[i], 9);
        copy_string(ui->clock_text[i], sizeof ui->clock_text[i],
                    text ? (const char *)text : "");
    }
}
