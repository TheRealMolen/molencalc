#pragma once

#include "platform.h"

//-------------------------------------------------------------------------------------------------

void hist_init();
void hist_enable_logging(bool enable);


void hist_add_line(const char* line);

const char* hist_get_curr_line();
int hist_count_lines();

// these return false when they're unable to move further
bool hist_prev();
bool hist_next();

void hist_jump_to_oldest();
void hist_jump_to_newest();

//-------------------------------------------------------------------------------------------------

