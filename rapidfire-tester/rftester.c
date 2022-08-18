#include <ngdevkit/neogeo.h>
#include <ngdevkit/ng-fix.h>
#include <ngdevkit/ng-video.h>
#include <stdio.h>
#include "version.h"

typedef enum {
    RFTESTER_STATE_IDLE,
    RFTESTER_STATE_START,
    RFTESTER_STATE_WAIT_NEXT,

    RFTESTER_STATE_MAX,  // must be last
} rftester_state_e;

typedef struct {
    rftester_state_e state;     // state machine
    volatile u16 time_count;    // counter incremented at each interrupt
    u16 time_limit_sec;         // count until how many seconds
    u16 time_increment;         // how much to increment time_count at each interrupt
    u16 vblank_hz;              // VBlank frequency
    u16 active_button_mask;     // active button used to time button hits
    u16 hit_count;              // how many times the active button pressed
} rftester_ctx_t;

rftester_ctx_t gCtx[1];

void rftester_init_palette(void) {
    const u16 palette[] = {
        0x8000, 0x0FFF,
    };

    for (u16 i = 0; i < sizeof(palette); i++) {
        MMAP_PALBANK1[i] = palette[i];
    }
}

void rftester_vblank_cb(void) {
    gCtx->time_count += gCtx->time_increment;
}

void rftester_print_info(void) {
    ng_cls();

    char str[30];
    const u16 col = 2;
    u16 row = 3;
    ng_text(col, row++, 0, "Rapid fire tester");

    snprintf(str, sizeof(str), " build %s", VERSION);
    ng_text(col, row++, 0, str);

    row = 22;
    ng_text(col, row++, 0, "Press any button to start count");
    ng_text(col, row++, 0, "Repeat same button until timer");

    snprintf(str, sizeof(str), "  reaches %u seconds", gCtx->time_limit_sec);
    ng_text(col, row++, 0, str);
    row++;
    ng_text(col, row++, 0, "2022 Sleepy Danny");
}

// player is 1-based counting
u8 rftester_current_buttons_for_player(u16 player) {
    const u8 current[] = {
        bios_p1current, bios_p2current, bios_p3current, bios_p4current,
    };

    return current[(player-1)];
}

// player is 1-based counting
u8 rftester_previous_buttons_for_player(u16 player) {
    const u8 previous[] = {
        bios_p1previous, bios_p2previous, bios_p3previous, bios_p4previous,
    };

    return previous[(player-1)];
}

// player is 1-based counting
u8 rftester_check_just_pressed_for_player(u16 player) {
    u8 current = rftester_current_buttons_for_player(player);
    u8 prev = rftester_previous_buttons_for_player(player);

    u8 mask = 0x1;
    while (mask) {
        if ((current & mask) && !(prev & mask)) {
            // button is currently pressed, previously not pressed
            break;
        }
        mask <<= 1;
    }

    return mask;
}

u16 rftester_check_just_pressed(void) {
    // support only player 1 & 2 for now
    return (rftester_check_just_pressed_for_player(2) << 8) |
           (rftester_check_just_pressed_for_player(1));
}

void rftester_print_hit_count(void) {
    char count_str[20];
    snprintf(count_str, sizeof(count_str), "Hits = %04u", gCtx->hit_count);
    ng_center_text(16, 0, count_str);
}

void rftester_print_time_count(void) {
    char time_str[20];
    snprintf(time_str, sizeof(time_str), "Time = %04u", gCtx->time_count / gCtx->vblank_hz);
    ng_center_text(15, 0, time_str);
}

void rftester_print_final_result(void) {
    char result_str[30];
    snprintf(result_str, sizeof(result_str), "%u hits in %u seconds",
             gCtx->hit_count, (gCtx->time_count / gCtx->vblank_hz));
    ng_center_text(10, 0, result_str);
}

void rftester_init(void) {
    gCtx->time_count = 0;
    gCtx->time_increment = 0;
    gCtx->time_limit_sec = 10;
    gCtx->vblank_hz = 60;
    gCtx->active_button_mask = 0;
    gCtx->hit_count = 0;
    gCtx->state = RFTESTER_STATE_IDLE;
    rftester_init_palette();
    rftester_print_info();
}

void rftester_process_state(void) {
    u16 pressed;

    switch (gCtx->state) {
        case RFTESTER_STATE_IDLE:
            pressed = rftester_check_just_pressed();
            if (pressed) {
                rftester_print_info();
                gCtx->hit_count = 1;
                gCtx->time_count = 0;
                gCtx->time_increment = 1;
                gCtx->active_button_mask = pressed;
                gCtx->state = RFTESTER_STATE_START;
                rftester_print_hit_count();
            }
            break;
        case RFTESTER_STATE_START: {
                pressed = rftester_check_just_pressed();
                // check if user pressed the same button as before
                if (pressed & gCtx->active_button_mask) {
                    gCtx->hit_count += 1;
                    rftester_print_hit_count();
                }

                const u16 time_limit = gCtx->vblank_hz * gCtx->time_limit_sec;
                if (gCtx->time_count >= time_limit) {
                    rftester_print_final_result();
                    gCtx->time_increment = 1;
                    gCtx->time_count = 0;
                    gCtx->state = RFTESTER_STATE_WAIT_NEXT;
                }
                rftester_print_time_count();
                break;
        }
        case RFTESTER_STATE_WAIT_NEXT: {
                // wait a little before starting next round
                const u16 time_limit = gCtx->vblank_hz * 2; // 2 sec
                if (gCtx->time_count >= time_limit) {
                    gCtx->state = RFTESTER_STATE_IDLE;
                }
                break;
        }
        default:
            break;
    }
    ng_wait_vblank();
}

