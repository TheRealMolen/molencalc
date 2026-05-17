//
//  PicoCalc multiline text input/output library with history
//
// originally based on parts of lcd.c from picocalc-text-starter

#include "text.h"

#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/spi.h"

#include "lcd.h"


static bool gMonospace = false;

static int gCursorX = 0;
static int gCursorY = 0;

#define MAX_COLS ((WIDTH)/2)
static int gCurrColIx = 0;
static uint8_t gColWidths[MAX_COLS];

static uint16_t gFgCol = 0xFF07;
static uint16_t gBgCol = 0x0000;

// Text drawing
const Font *gFont = &font_10x16;
static uint16_t char_buffer[16 * FONT_MAX_HEIGHT] __attribute__((aligned(4)));

static repeating_timer_t cursor_timer;


void text_set_font(const Font *new_font)
{
    // Set the new font
    gFont = new_font;
}

// Set foreground colour
void text_set_foreground(uint16_t colour)
{
    gFgCol = colour;
}

// Set background colour
void text_set_background(uint16_t colour)
{
    gBgCol = colour;
}

void text_set_monospace(bool mono)
{
    gMonospace = mono;
}

// Draw a character at the specified position
// returns the width of the drawn character
uint8_t text_putc(int x, int y, uint8_t c)
{
    const int glyph_width = gFont->Width;
    const int glyph_height = gFont->Height;
    font_rasterise_char(gFont, c, gFgCol, gBgCol, char_buffer, glyph_width, glyph_height, 0, 0);

    const GlyphMetric metric = font_get_glyph_metric(gFont, c, gMonospace);

    // if we have any skipping/shrinking to do, do that inplace
    if (metric.Skip > 0 || metric.Advance < glyph_width)
    {
        uint16_t* dest = char_buffer;
        const uint16_t* src = char_buffer + metric.Skip;
        for (int row = 0; row < glyph_height; ++row)
        {
            memmove(dest, src, metric.Advance * sizeof(*src));
            dest += metric.Advance;
            src += glyph_width;
        }
    }

    // the buf width is the smaller of the glyph_width and Advance
    const int bufw = (glyph_width < metric.Advance) ? glyph_width : metric.Advance;
    lcd_blit(char_buffer, x, y, bufw, glyph_height);

    return metric.Advance;
}




void text_put_image(const uint16_t* pixels, uint32_t imgw, uint32_t imgh) 
{
    // we draw the image line-by-line
    // partly because it makes it animate nice, and partly because 
    // figuring out the appropriate logic to wrap the image around the frame
    // is more effort than i'm interested in :)
    uint32_t height_remaining = imgh;

    const uint16_t* next_pixels = pixels;
    while (height_remaining > 0)
    {
        uint32_t line_height = 1;

        // scroll up enough so there's at least imgh pixels free to draw on 
        // nb. we're over-clearing the back buf at this point as we're about to blat over a chunk with the img 
        int line_btm = gCursorY; 
        int img_top = HEIGHT - line_height; 
        if (line_btm > img_top) 
        { 
            lcd_scroll_up(line_btm - img_top); 
            line_btm = gCursorY; 
        }
        if (img_top > line_btm)
            img_top = line_btm;

        const int img_left = (int)(WIDTH - imgw - 1);

        lcd_blit(next_pixels, img_left, img_top, imgw, line_height);
    
        gCursorY += line_height;
        height_remaining -= line_height;
        next_pixels += imgw * line_height;
    }
} 




void text_inc_column(uint8_t advance)
{
    gCursorX += advance;

    gColWidths[gCurrColIx] = advance;
    ++gCurrColIx;

    if (gCursorX >= WIDTH || gCurrColIx >= MAX_COLS)
    {
        gCursorX = 0;
        gCursorY += gFont->Height;

        // TODO: this breaks backspace from one line to the previous
        // ideally we'd remember the start ix of each line and only reset when flushing
        gCurrColIx = 0;
    }
}

void text_backspace()
{
    if (gCurrColIx <= 0)
        return;

    cursor_erase();

    --gCurrColIx;

    const int glyphWidth = gColWidths[gCurrColIx];
    gCursorX -= glyphWidth;

    lcd_solid_rectangle(gBgCol, gCursorX, gCursorY, glyphWidth, gFont->Height);

    cursor_draw();
}


static void text_next_line()
{
    const int glyph_height = gFont->Height;
    gCursorY += glyph_height;

    while (gCursorY >= (HEIGHT - glyph_height))
        text_scroll_up();
}

static void text_next_tab()
{
    const int tabwidth = 6 * gFont->Width;
    gCursorX += tabwidth + gFont->Width - 1;
    if (gCursorX >= (WIDTH - tabwidth))
    {
        gCursorX = 0;
        gCurrColIx = 0;
        text_next_line();
    }
    else
    {
        gCursorX -= (gCursorX % tabwidth);
    }
}



void text_emit(char c)
{
    cursor_erase(); // erase the cursor before processing the character

    switch (c)
    {
    case '\b':
        text_backspace();
        break;

    case '\t':
        text_next_tab();
        break;

    case '\n':
        gCurrColIx = 0;
        gCursorX = 0;
        text_next_line();

    default:
        if (c >= 0x20 && c < 0x7F) // printable characters
        {
            const uint8_t advance = text_putc(gCursorX, gCursorY, c);
            text_inc_column(advance);
        }
        break;
    }

    cursor_draw();
}

void text_emit_str(const char* s)
{
    if (!s)
        return;
    
    for (; *s; ++s)
        text_emit(*s);
}


static bool cursor_enabled = true; // cursor visibility state

// Enable or disable the cursor
void cursor_enable(bool cursor_on)
{
    // Cursor visibility is not implemented, but we can toggle the state
    cursor_enabled = cursor_on;
}

// Check if the cursor is enabled
bool cursor_is_enabled()
{
    // Return the current cursor visibility state
    return cursor_enabled;
}


static void blit_cursor(uint16_t col)
{
    if (!cursor_enabled)
        return;

    lcd_solid_rectangle(col, gCursorX, gCursorY + gFont->Height - 1, gFont->Width, 1);
}

// Draw the cursor at the current position
void cursor_draw()
{
    blit_cursor(gFgCol);
}

// Erase the cursor at the current position
void cursor_erase()
{
    blit_cursor(gBgCol);
}


//
//  Background processing
//
//  Handle background tasks such as blinking the cursor
//

// Blink the cursor at regular intervals
bool on_cursor_timer(repeating_timer_t *rt)
{
    static bool cursor_visible = false;

    if (!cursor_is_enabled())
    {
        return true; // if the SPI bus is not available or cursor is disabled, do not toggle cursor
    }

    if (cursor_visible)
    {
        cursor_erase();
    }
    else
    {
        cursor_draw();
    }

    cursor_visible = !cursor_visible; // Toggle cursor visibility
    return true;                      // Keep the timer running
}



void text_scroll_up()
{
    cursor_erase();

    const int distance = gFont->Height;

    lcd_scroll_up(distance);

    if (gCursorY > distance)
        gCursorY -= distance;
    else
        gCursorY = 0;
}

void text_scroll_down()
{
    cursor_erase();

    const int distance = gFont->Height;

    lcd_scroll_down(distance);

    if (gCursorY + distance >= HEIGHT)
        gCursorY = HEIGHT - distance;
    else
        gCursorY -= distance;
}

void text_init()
{
    // Blink the cursor every second (500 ms on, 500 ms off)
    add_repeating_timer_ms(-500, on_cursor_timer, NULL, &cursor_timer);
}
