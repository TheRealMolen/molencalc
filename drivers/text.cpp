//
//  PicoCalc multiline text input/output library with history
//
// originally based on parts of lcd.c from picocalc-text-starter

#include "text.h"

#include <string.h>

#include "colours.h"
#include "font.h"
#include "keyboard.h"
#include "lcd.h"
#include "palette.h"
#include "platform.h"

#if MLN_TARGET_PICO
#include "pico/stdlib.h"
#endif

//----------------------------------------------------------------------------------------
// Text drawing
const Font *gFont = &font_10x16;

#if LCD_USEPALETTE
static col_t gFgCol = PAL_FG;
static col_t gBgCol = PAL_BG;
#else
static col_t gFgCol = COL_DEFAULT_FG;
static col_t gBgCol = COL_DEFAULT_BG;
#endif

static bool gMonospace = false;

//----------------------------------------------------------------------------------------
// Cursor
static int gCursorX = 2;
static int gCursorY = 2;
static int8_t gCursorWidth = gFont->Width;

static bool gCursorEnabled = true; // cursor visibility state

#if MLN_TARGET_PICO
static repeating_timer_t gCursorTimer;
#endif

//----------------------------------------------------------------------------------------
// Line editing
//

// this info is all about the current input "line". that "line" can span multiple screen lines
constexpr int kMaxInputLen = 255;

char gInputBuf[kMaxInputLen + 1] = {0}; // the input read buffer
int16_t gInputEditIx = 0;               // the index of the cursor within the readbuf
int16_t gInputLen = 0;                  // the index of the final character in the readbuf
int16_t gInputStartY = 0;                // the y value of the start of this input line. used when clearing and resetting.

bool gInputTextComplete = false;        // set to true when gInputBuf[gInputLen] has been set to 0 to terminate the string

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

void history_add_line(const char* line);
const char* history_prev();
const char* history_next();

//----------------------------------------------------------------------------------------

static inline constexpr bool is_printable(char c)
{
    return (c >= ' ') && (c < 0x7f);
}

static inline bool in_edit_mode()
{
    return gInputEditIx != gInputLen;
}

//----------------------------------------------------------------------------------------

void text_set_font(const Font *new_font)
{
    gFont = new_font;
}

void text_set_foreground(col_t colour)
{
    gFgCol = colour;
}

void text_set_background(col_t colour)
{
    gBgCol = colour;
}

col_t text_get_foreground()
{
    return gFgCol;
}
col_t text_get_background()
{
    return gBgCol;
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

    lcd_scroll_up(distance, gBgCol);

    const int startY = gCursorY;
    if (gCursorY > distance)
        gCursorY -= distance;
    else
        gCursorY = 0;

    gInputStartY -= (gCursorY - startY);
}

//----------------------------------------------------------------------------------------

// Draw a character at the specified position
// returns the width of the drawn character
uint8_t text_putc(int x, int y, uint8_t c)
{
    col_t char_buffer[16 * FONT_MAX_HEIGHT] __attribute__((aligned(4)));

    const int glyph_width = gFont->Width;
    const int glyph_height = gFont->Height;
    font_rasterise_char(gFont, c, gFgCol, gBgCol, char_buffer, glyph_width, glyph_height, 0, 0);

    const GlyphMetric metric = font_get_glyph_metric(gFont, c, gMonospace);

    // if we have any skipping/shrinking to do, do that inplace
    if (metric.Skip > 0 || metric.Advance < glyph_width)
    {
        col_t* dest = char_buffer;
        const col_t* src = char_buffer + metric.Skip;
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


void text_put_image(const col_t* pixels, uint32_t imgw, uint32_t imgh) 
{
    cursor_erase();

    // we draw the image line-by-line
    // partly because it makes it animate nice, and partly because 
    // figuring out the appropriate logic to wrap the image around the frame
    // is more effort than i'm interested in :)
    uint32_t height_remaining = imgh;

    const col_t* next_pixels = pixels;
    while (height_remaining > 0)
    {
        constexpr int line_height = 1;
        if (gCursorY >= HEIGHT - line_height) 
        { 
            lcd_scroll_up(1); 
            gCursorY -= 1;
            gInputStartY -= 1;
        }

        const int img_left = (int)(WIDTH - imgw - 1);

        lcd_rect(0, gCursorY, img_left - 1, 1, gBgCol);
        lcd_blit(next_pixels, img_left, gCursorY, imgw, line_height);
    
        gCursorY += line_height;
        height_remaining -= line_height;
        next_pixels += imgw * line_height;
    }
} 


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void text_inc_column(uint8_t advance)
{
    if (gInputLen >= kMaxInputLen)
        return;

    gCursorX += advance;

    if (gCursorX >= WIDTH)
    {
        gCursorX = 0;

        gCursorY += gFont->Height;
        if (gCursorY >= HEIGHT)
            text_scroll_up();
    }
}

static void text_next_line()
{
    const int glyph_height = gFont->Height;
    gCursorY += glyph_height;

    while (gCursorY >= (HEIGHT - glyph_height))
        text_scroll_up();

    gCursorX = 0;

    gInputStartY = gCursorY;
}

static void text_next_tab()
{
    const int tabwidth = 6 * gFont->Width;
    gCursorX += tabwidth + gFont->Width - 1;
    if (gCursorX >= (WIDTH - tabwidth))
    {
        gCursorX = 0;
        text_next_line();
    }
    else
    {
        gCursorX -= (gCursorX % tabwidth);
    }
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void _emit_char(char c)
{
    switch (c)
    {
    //case '\b':  text_backspace();   return;
    case '\t':  text_next_tab();    return;
    case '\n':  text_next_line();   return;
    }

    if (!is_printable(c))
        return;

    // if this char would end off the screen, advance to the next line immediately
    const GlyphMetric metric = font_get_glyph_metric(gFont, c, gMonospace);
    if (gCursorX + metric.Advance >= WIDTH)
    {
        text_inc_column(metric.Advance);
    }

    const uint8_t advance = text_putc(gCursorX, gCursorY, c);
    text_inc_column(advance);
}


void text_emit(char c)
{
    cursor_erase(); // erase the cursor before processing the character

    _emit_char(c);

    cursor_draw();
}

void text_emit_str(const char* s)
{
    if (!s)
        return;
    
    cursor_erase();

    for (; *s; ++s)
        _emit_char(*s);

    cursor_draw();
}

void text_clear_line(const char* prompt)
{
    int input_height = (gCursorY - gInputStartY) + gFont->Height;
    lcd_rect(0, gInputStartY, WIDTH, input_height, gBgCol);

    gCursorX = 0;
    gCursorY = gInputStartY;

    if (prompt)
        text_emit_str(prompt);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// C U R S O R
//

// Enable or disable the cursor
void cursor_enable(bool cursor_on)
{
    // Cursor visibility is not implemented, but we can toggle the state
    gCursorEnabled = cursor_on;
}

// Check if the cursor is enabled
bool cursor_is_enabled()
{
    // Return the current cursor visibility state
    return gCursorEnabled;
}


static void blit_cursor(col_t col)
{
    if (!gCursorEnabled)
        return;

    lcd_rect(gCursorX, gCursorY + gFont->Height - 1, gCursorWidth, 1, col);
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

static void input_redraw()
{
    cursor_erase();
    text_clear_line(">");

    int cursX = gCursorX;
    int cursY = gCursorY;

    for (int i=0; i<gInputLen; ++i)
    {
        if (i == gInputEditIx)
        {
            cursX = gCursorX;
            cursY = gCursorY;
        }

        _emit_char(gInputBuf[i]);
    }

    if (in_edit_mode())
    {
        gCursorX = cursX;
        gCursorY = cursY;

        const char c = gInputBuf[gInputEditIx];
        const GlyphMetric metric = font_get_glyph_metric(gFont, c, gMonospace);
        gCursorWidth = metric.Advance;
    }
    else
    {
        gCursorWidth = gFont->Width;
    } 

    cursor_draw();
}

static void input_delete_prev_char()
{
    if (gInputEditIx <= 0)
        return;

    if (gInputEditIx < gInputLen)
    {
        memmove(gInputBuf + gInputEditIx - 1, gInputBuf + gInputEditIx, gInputLen - gInputEditIx);
    }

    --gInputEditIx;
    --gInputLen;
}

void input_move_cursor_left()
{
    if (gInputEditIx > 0)
        --gInputEditIx;
    else
        gInputEditIx = 0;
}

void input_move_cursor_right()
{
    if (gInputEditIx < gInputLen)
        ++gInputEditIx;
    else
        gInputEditIx = gInputLen;
}

// adds a (printable) character to the current cursor pos in our input line
static void input_enter_char(char c)
{
    if (gInputLen+1 >= kMaxInputLen)
        return;

    if (in_edit_mode())
    {
        memmove(gInputBuf + gInputEditIx + 1, gInputBuf + gInputEditIx, gInputLen - gInputEditIx);
    }

    gInputBuf[gInputEditIx] = c;
    ++gInputEditIx;
    ++gInputLen;

    if (in_edit_mode())
    {
        input_redraw();
    }
    else
    {
        // just append the character
        text_emit(c);
    }
}

void input_process_char(int c)
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
        gInputBuf[gInputLen] = 0;
        gInputTextComplete = true;
        return;

    case KEY_BACKSPACE:
        input_delete_prev_char();
        input_redraw();
        break;
    case KEY_DEL:
        if (in_edit_mode())
        {
            input_move_cursor_right();
            input_delete_prev_char();
            input_redraw();
        }
        break;

    case KEY_UP:
        //TODO: writeme
        gInputEditIx = gInputLen = 0;
        text_clear_line(">");
        break;
    case KEY_DOWN:
        //TODO: writeme
        break;

    case KEY_LEFT:
        input_move_cursor_left();
        input_redraw();
        break;
    case KEY_RIGHT:
        input_move_cursor_right();
        input_redraw();
        break;

    default:
        if (is_printable(c))
        {
            input_enter_char(c);
        }
    }
}


bool input_has_complete_line()
{
    return gInputTextComplete;
}

const char* input_get_line()
{
    return gInputBuf;
}

void input_reset_line()
{
    //TODO: history_append_line(input_get_line());

    gInputEditIx = 0;
    gInputLen = 0;
    gInputBuf[0] = 0;
    gInputTextComplete = false;

    text_emit_str("\n>");
}

//-------------------------------------------------------------------------------------------------

void history_add_line(const char* line)
{
    (void)line;
}

const char* history_prev()
{
    return nullptr;
}

const char* history_next()
{
    return nullptr;
}

//-------------------------------------------------------------------------------------------------

void text_init()
{
    for (HistoryLine& line : gHistoryLines)
        line.Valid = false;

#if MLN_TARGET_PICO
    // Blink the cursor every second (500 ms on, 500 ms off)
    add_repeating_timer_ms(-500, on_cursor_timer, NULL, &gCursorTimer);
#endif
}

//-------------------------------------------------------------------------------------------------

