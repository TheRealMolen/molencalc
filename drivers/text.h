#pragma once

#include <cstdint>

//-------------------------------------------------------------------------------------------------

typedef struct Font Font;

//-------------------------------------------------------------------------------------------------

void text_init();

void text_scroll_up();
void text_scroll_down();


void text_set_foreground(uint16_t colour);
void text_set_background(uint16_t colour);
void text_set_monospace(bool mono);
void text_set_font(const Font *new_font);

// Draw a character at the specified position
// returns the width of the drawn character
uint8_t text_putc(int x, int y, uint8_t c);

void text_put_image(const uint16_t* pixels, uint32_t imgw, uint32_t imgh);

void text_inc_column(uint8_t advance);
void text_backspace();

void text_emit(char c);
void text_emit_str(const char* s);

//-------------------------------------------------------------------------------------------------

void cursor_draw();
void cursor_erase();
void cursor_enable(bool cursor_on);
bool cursor_is_enabled();

//-------------------------------------------------------------------------------------------------

void input_process_char(char c);
bool input_has_complete_line();
const char* input_get_line();
void input_reset_line();

//-------------------------------------------------------------------------------------------------

