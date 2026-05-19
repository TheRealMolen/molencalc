//
//  PicoCalc multiline text input/output library with history
//
// originally based on parts of lcd.c from picocalc-text-starter

#include "text.h"

#include <string.h>

#include "font.h"
#include "keyboard.h"
#include "lcd.h"
#include "platform.h"

#if MLN_TARGET_PICO
#include "pico/stdlib.h"
#endif

//----------------------------------------------------------------------------------------
// Text drawing
const Font *gFont = &font_10x16;

static uint16_t char_buffer[16 * FONT_MAX_HEIGHT] __attribute__((aligned(4)));

static uint16_t gFgCol = 0xFF07;
static uint16_t gBgCol = 0x0000;

static bool gMonospace = false;

//----------------------------------------------------------------------------------------
// Cursor
static int gCursorX = 0;
static int gCursorY = 0;
static int8_t gCursorWidth = gFont->Width;

#if MLN_TARGET_PICO
static repeating_timer_t cursor_timer;
#endif

//----------------------------------------------------------------------------------------
// Line editing
constexpr int kMaxColIx = (WIDTH/2) - 1;
constexpr int kMaxLinesInBuf = 16;
int8_t gColWidths[kMaxColIx+1];
uint16_t gLineEndX[kMaxLinesInBuf];
int16_t gCurrColIx = 0;
int16_t gCurrLineIx = 0;

constexpr int kReadBufSize = 256;
int16_t gReadBufIx = 0;     // the index of the cursor within the readbuf
int16_t gReadBufEndIx = 0;  // the index of the final character in the readbuf
bool gReadBufComplete = false;
char gReadBuf[kReadBufSize] = {0};

//----------------------------------------------------------------------------------------
// Input history

// the line history buffer is in two parts:
//  1. a rolling buffer of chars excluding newlines and terminators; just a big block of chars
//  2. ringbuffer of indices into the above + lengths
constexpr int kHistoryBufSize = 2 * 1024;
constexpr int kHistoryMaxLines = 32;

struct HistoryLine
{
    uint16_t StartIx = 0;
    uint16_t Length = 0;
    bool     Valid = false;
};

char gHistoryBuf[kHistoryBufSize] = {0};
int gNextHistoryWriteChar = 0;
HistoryLine gHistoryLines[kHistoryMaxLines];
int gCurrHistoryLine = 0;


//----------------------------------------------------------------------------------------

void text_set_font(const Font *new_font)
{
    gFont = new_font;
}

void text_set_foreground(uint16_t colour)
{
    gFgCol = colour;
}

void text_set_background(uint16_t colour)
{
    gBgCol = colour;
}

void text_set_monospace(bool mono)
{
    gMonospace = mono;
}

//----------------------------------------------------------------------------------------

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
        gCursorY += distance;
}

//----------------------------------------------------------------------------------------

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
        constexpr int line_height = 1;
        if (gCursorY >= HEIGHT - line_height) 
        { 
            lcd_scroll_up(1); 
            gCursorY -= 1;
        }

        const int img_left = (int)(WIDTH - imgw - 1);

        lcd_blit(next_pixels, img_left, gCursorY, imgw, line_height);
    
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

    if (gCursorX >= WIDTH || gCurrColIx >= kMaxColIx)
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

    lcd_rect(gCursorX, gCursorY, glyphWidth, gFont->Height, gBgCol);

    cursor_draw();
}


static void text_next_line()
{
    const int glyph_height = gFont->Height;
    gCursorY += glyph_height;

    while (gCursorY >= (HEIGHT - glyph_height))
        text_scroll_up();

    gCurrColIx = 0;
    gCursorX = 0;
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
        text_next_line();
        break;

    default:
        if (c >= 0x20 && c < 0x7F) // printable characters
        {
            // TODO: refactor this; should all be in a single call!

            // if this char would end off the screen, advance to the next line immediately
            const GlyphMetric metric = font_get_glyph_metric(gFont, c, gMonospace);
            if (gCursorX + metric.Advance >= WIDTH)
            {
                text_inc_column(metric.Advance);
            }

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

    lcd_rect(gCursorX, gCursorY + gFont->Height - 1, gFont->Width, 1, col);
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

#if MLN_TARGET_PICO

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

#endif


//-------------------------------------------------------------------------------------------------

static void input_delete_prev_char()
{
    if (gReadBufIx > 0)
    {
        if (gReadBufIx < gReadBufEndIx)
        {
            memmove(gReadBuf + gReadBufIx, gReadBuf + gReadBufIx + 1, gReadBufEndIx - gReadBufIx);
        }

        --gReadBufIx;
        --gReadBufEndIx;

        text_backspace();
    }
}

void input_move_cursor_left()
{
    //TODO:
    if (gReadBufIx > 0)
return;
}

void input_move_cursor_right()
{
    //TODO:
}

// adds a (printable) character to the current cursor pos in our input line
static void input_enter_char(char c)
{
    if (gReadBufIx < gReadBufEndIx)
    {
        memmove(gReadBuf + gReadBufIx + 1, gReadBuf + gReadBufIx, gReadBufEndIx - gReadBufIx);
    }

    gReadBuf[gReadBufIx] = c;
    ++gReadBufIx;
    ++gReadBufEndIx;

    text_emit(c);
}

void input_process_char(char c)
{
    if (input_has_complete_line())
        return;

    switch (c)
    {
    //case '\r': case '\n':
    case KEY_RETURN:
        cursor_erase();
        text_next_line();

        // null-terminate and reremember that we have a complete line
        gReadBuf[gReadBufEndIx] = 0;
        gReadBufComplete = true;
        return;

    case KEY_BACKSPACE:
        input_delete_prev_char();
        break;
    case KEY_DEL:
        input_move_cursor_right();
        input_delete_prev_char();
        break;

    default:
        if ((gReadBufIx+1 < kReadBufSize) && (c >= 0x20) && (c < 0x7f))
        {
            input_enter_char(c);
        }
    }
}


bool input_has_complete_line()
{
    return gReadBufComplete;
}

const char* input_get_line()
{
    return gReadBuf;
}

void input_reset_line()
{
    //TODO: history_append_line(input_get_line());

    gReadBufIx = 0;
    gReadBufEndIx = 0;
    gReadBuf[0] = 0;
    gReadBufComplete = false;

    text_emit_str("\n>");
}

//-------------------------------------------------------------------------------------------------

void text_init()
{
    for (HistoryLine& line : gHistoryLines)
        line.Valid = false;

#if MLN_TARGET_PICO
    // Blink the cursor every second (500 ms on, 500 ms off)
    add_repeating_timer_ms(-500, on_cursor_timer, NULL, &cursor_timer);
#endif
}

//-------------------------------------------------------------------------------------------------

