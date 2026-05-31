#pragma once

#include <cstdint>

#include "colours.h"

//-------------------------------------------------------------------------------------------------

typedef struct Font Font;
typedef struct Palette Palette;

//-------------------------------------------------------------------------------------------------

void text_init();

void text_scroll_up();

void text_set_foreground(col_t colour);
void text_set_background(col_t colour);
col_t text_get_foreground();
col_t text_get_background();
void text_set_monospace(bool mono);
void text_set_font(const Font *new_font);
const Font* text_get_font();

// Draw a character at the specified position
// returns the width of the drawn character
uint8_t text_putc(int x, int y, uint8_t c);

void text_put_image(const col_t* pixels, uint32_t imgw, uint32_t imgh);

void text_inc_column(uint8_t advance);
void text_backspace();

void text_emit(char c);
void text_emit_str(const char* s);

// wipe any in-progress text input and optionally display the supplied prompt
void text_clear_line(const char* prompt);

//-------------------------------------------------------------------------------------------------

void cursor_draw();
void cursor_erase();
void cursor_enable(bool cursor_on);
bool cursor_is_enabled();

//-------------------------------------------------------------------------------------------------

void input_process_char(int c);
bool input_has_complete_line();
const char* input_get_line();
void input_reset_line();

//-------------------------------------------------------------------------------------------------

