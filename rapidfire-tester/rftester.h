#pragma once

// initialization
void rftester_init(void);

// VBlank interrupt callback
void rftester_vblank_cb(void);

// main state machine
void rftester_process_state(void);

