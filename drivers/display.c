//
//  PicoCalc LCD display driver
//
//  This driver implements a simple VT100 terminal interface for the PicoCalc LCD display
//  using the ST7789P LCD controller.
//
//  It is optimised for a character-based display with a fixed-width, 8-pixel wide font
//  and 65K colours in the RGB565 format. This driver requires little memory as it
//  uses the frame memory on the controller directly.
//
//  NOTE: Some code below is written to respect timing constraints of the ST7789P controller.
//        For instance, you can usually get away with a short chip select high pulse widths, but
//        writing to the display RAM requires the minimum chip select high pulse width of 40ns.
//

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/spi.h"

#include "lcd.h"
#include "display.h"

// Colour Palette definitions
//
// The RGB() macro can be used to create colors from this colour depth using 8-bits per
// channel values.

// 3-bit terminal colours (based on VS Code)
const uint16_t palette[8] = {
    RGB(0, 0, 0),      // Black
    RGB(205, 0, 0),    // Red
    RGB(0, 205, 0),    // Green
    RGB(205, 205, 0),  // Yellow
    RGB(0, 0, 238),    // Blue
    RGB(205, 0, 205),  // Magenta
    RGB(0, 205, 205),  // Cyan
    RGB(229, 229, 229) // White
};

// Bright colours for the 3-bit terminal colours (based on VS Code)
const uint16_t bright_palette[8] = {
    RGB(127, 127, 127), // Bright Black (Gray)
    RGB(255, 0, 0),     // Bright Red
    RGB(0, 255, 0),     // Bright Green
    RGB(255, 255, 0),   // Bright Yellow
    RGB(92, 92, 255),   // Bright Blue
    RGB(255, 0, 255),   // Bright Magenta
    RGB(0, 255, 255),   // Bright Cyan
    RGB(255, 255, 255)  // Bright White
};

// Xterm 256-colour palette (RGB565 format)
//
// That is 5-bits for red and blue, and 6-bits for green. The display is configured to map to
// its native 18-bit RGB using the LSB of green for the LSB of red and blue, and the green
// component is unmodified.

const uint16_t xterm_palette[256] = {
    // Standard 16 colors (0-15)
    0x0000, 0x8000, 0x0400, 0x8400, 0x0010, 0x8010, 0x0410, 0xC618,
    0x8410, 0xF800, 0x07E0, 0xFFE0, 0x001F, 0xF81F, 0x07FF, 0xFFFF,

    // 216 colors in 6×6×6 RGB cube (16-231)
    0x0000, 0x0010, 0x0015, 0x001F, 0x0014, 0x001F, 0x0400, 0x0410, 0x0415, 0x041F, 0x0414, 0x041F,
    0x0500, 0x0510, 0x0515, 0x051F, 0x0514, 0x051F, 0x07E0, 0x07F0, 0x07F5, 0x07FF, 0x07F4, 0x07FF,
    0x0600, 0x0610, 0x0615, 0x061F, 0x0614, 0x061F, 0x07E0, 0x07F0, 0x07F5, 0x07FF, 0x07F4, 0x07FF,
    0x8000, 0x8010, 0x8015, 0x801F, 0x8014, 0x801F, 0x8400, 0x8410, 0x8415, 0x841F, 0x8414, 0x841F,
    0x8500, 0x8510, 0x8515, 0x851F, 0x8514, 0x851F, 0x87E0, 0x87F0, 0x87F5, 0x87FF, 0x87F4, 0x87FF,
    0x8600, 0x8610, 0x8615, 0x861F, 0x8614, 0x861F, 0x87E0, 0x87F0, 0x87F5, 0x87FF, 0x87F4, 0x87FF,
    0xA000, 0xA010, 0xA015, 0xA01F, 0xA014, 0xA01F, 0xA400, 0xA410, 0xA415, 0xA41F, 0xA414, 0xA41F,
    0xA500, 0xA510, 0xA515, 0xA51F, 0xA514, 0xA51F, 0xA7E0, 0xA7F0, 0xA7F5, 0xA7FF, 0xA7F4, 0xA7FF,
    0xA600, 0xA610, 0xA615, 0xA61F, 0xA614, 0xA61F, 0xA7E0, 0xA7F0, 0xA7F5, 0xA7FF, 0xA7F4, 0xA7FF,
    0xF800, 0xF810, 0xF815, 0xF81F, 0xF814, 0xF81F, 0xFC00, 0xFC10, 0xFC15, 0xFC1F, 0xFC14, 0xFC1F,
    0xFD00, 0xFD10, 0xFD15, 0xFD1F, 0xFD14, 0xFD1F, 0xFFE0, 0xFFF0, 0xFFF5, 0xFFFF, 0xFFF4, 0xFFFF,
    0xFE00, 0xFE10, 0xFE15, 0xFE1F, 0xFE14, 0xFE1F, 0xFFE0, 0xFFF0, 0xFFF5, 0xFFFF, 0xFFF4, 0xFFFF,
    0xC000, 0xC010, 0xC015, 0xC01F, 0xC014, 0xC01F, 0xC400, 0xC410, 0xC415, 0xC41F, 0xC414, 0xC41F,
    0xC500, 0xC510, 0xC515, 0xC51F, 0xC514, 0xC51F, 0xC7E0, 0xC7F0, 0xC7F5, 0xC7FF, 0xC7F4, 0xC7FF,
    0xC600, 0xC610, 0xC615, 0xC61F, 0xC614, 0xC61F, 0xC7E0, 0xC7F0, 0xC7F5, 0xC7FF, 0xC7F4, 0xC7FF,
    0xE000, 0xE010, 0xE015, 0xE01F, 0xE014, 0xE01F, 0xE400, 0xE410, 0xE415, 0xE41F, 0xE414, 0xE41F,
    0xE500, 0xE510, 0xE515, 0xE51F, 0xE514, 0xE51F, 0xE7E0, 0xE7F0, 0xE7F5, 0xE7FF, 0xE7F4, 0xE7FF,
    0xE600, 0xE610, 0xE615, 0xE61F, 0xE614, 0xE61F, 0xE7E0, 0xE7F0, 0xE7F5, 0xE7FF, 0xE7F4, 0xE7FF,

    // 24 grayscale colors (232-255)
    0x0000, 0x1082, 0x2104, 0x3186, 0x4208, 0x528A, 0x630C, 0x738E,
    0x8410, 0x9492, 0xA514, 0xB596, 0xC618, 0xD69A, 0xE71C, 0xF79E,
    0x0841, 0x18C3, 0x2945, 0x39C7, 0x4A49, 0x5ACB, 0x6B4D, 0x7BCF};

//
//  VT100 Terminal Emulation
//
//  This section contains the definitions and variables used for handling
//  ANSI escape sequences, cursor positioning, and text attributes.
//
//  This implementation is lacking full support for the VT100 terminal.
//
//  Reference: https://vt100.net/docs/vt100-ug/chapter3.html
//

bool tab_stops[64] = {0};
uint8_t debug[64] = {0};
int debug_index = 0;

uint8_t state = STATE_NORMAL; // initial state of escape sequence processing
uint8_t column = 0;           // cursor x position
uint8_t row = 0;              // cursor y position

uint16_t parameters[16]; // buffer for selective parameters
uint8_t p_index = 0;    // index into the buffer

uint8_t save_column = 0; // saved cursor x position for DECSC/DECRC
uint8_t save_row = 0;    // saved cursor y position for DECSC/DECRC
uint8_t leds = 0;        // current LED state

uint8_t g0_charset = CHARSET_ASCII; // G0 character set (default ASCII)
uint8_t g1_charset = CHARSET_ASCII; // G1 character set (default ASCII)
uint8_t active_charset = 0;         // currently active character set (0=G0, 1=G1)

void (*display_led_callback)(uint8_t) = NULL;
void (*display_bell_callback)(void) = NULL;
void (*display_report_callback)(const char *) = NULL;


static void ring_bell()
{
    if (display_bell_callback)
    {
        display_bell_callback(); // Call the user-defined bell callback
    }
}

//
// Display API
//

bool display_emit_available()
{
    return true; // always available for output in this implementation
}

void display_emit(char ch)
{
    int max_row = MAX_ROW;
    int max_col = lcd_get_columns() - 1;

    lcd_erase_cursor(); // erase the cursor before processing the character

    switch (ch)
    {
    case CHR_BS:
        column = MAX(0, column - 1); // move cursor back one space (but not before the start of the line)
        break;
    case CHR_BEL:
        ring_bell(); // ring the bell
        break;
    case CHR_HT:
        column = MIN(((column + 8) & ~7), lcd_get_columns() - 1); // move cursor to next tabstop (but not beyond the end of the line)
        break;
    case CHR_LF:
    case CHR_VT:
    case CHR_FF:
        row++; // move cursor down one line
        break;
    case CHR_CR:
        column = 0; // move cursor to the start of the line
        break;

    default:
        if (ch >= 0x20 && ch < 0x7F) // printable characters
        {
            lcd_putc(column++, row, ch);
        }
        break;
    }

    // Handle wrapping and scrolling
    if (column > max_col) // wrap around at end of the line
    {
        column = 0;
        row++;
    }

    if (row > max_row) // scroll at bottom of the screen
    {
        while (row > max_row) // scroll until y is within bounds
        {
            lcd_scroll_up(); // scroll up to make space at the bottom
            row--;
        }
    }

    // Update cursor position
    lcd_move_cursor(column, row);
    lcd_draw_cursor(); // draw the cursor at the new position
}

//
//  Display Callback Setters
//

void display_set_led_callback(led_callback_t callback)
{
    display_led_callback = callback;
}

void display_set_bell_callback(bell_callback_t callback)
{
    display_bell_callback = callback;
}

void display_set_report_callback(report_callback_t callback)
{
    display_report_callback = callback;
}

//
//  Display Initialization
//

void display_init()
{
    // Make sure the LCD is initialized
    lcd_init();

    // Set tab stops every 8 columns by default
    for (int i = 3; i < 64; i += 8)
    {
        tab_stops[i] = true;
    }
}