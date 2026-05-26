#include "gfx.h"

#include "colours.h"
#include "lcd.h"
#include "palette.h"

#include <cstring>

//----------------------------------------------------------------------------------------

#if GFX_USEFRAMEBUF
static col_t gFramebuf[WIDTH*BACKBUF_HEIGHT];
static bool gAllowFramebufUpdate = true;
#endif

static const Palette* gActivePalette = nullptr;

//----------------------------------------------------------------------------------------

void gfx_set_palette(const Palette* palette)
{
    if (palette == gActivePalette)
        return;

    [[maybe_unused]] const bool wasInvalid = gActivePalette == nullptr;

    gActivePalette = palette;

#if GFX_USEFRAMEBUF
    // if we're using a framebuf and we aren't initialising, we can redraw the whole screen in the new palette
    if (wasInvalid)
        return;

    // ...but lcd_blit will try and write to the fb, so we can suppress that
    gAllowFramebufUpdate = false;

    const int lcd_y_offset = lcd_get_scroll_offset();
    int y_framebuf = lcd_y_offset;
    for (int y = 0; y < HEIGHT; ++y)
    {
        const col8_t* line = gFramebuf + (y_framebuf * WIDTH);

        lcd_blit(line, 0, y, WIDTH, 1);

        ++y_framebuf;
        if (y_framebuf >= BACKBUF_HEIGHT)
            y_framebuf -= BACKBUF_HEIGHT;
    }    

    gAllowFramebufUpdate = true;
#endif
}

const Palette* gfx_get_palette()
{
    return gActivePalette;
}

//----------------------------------------------------------------------------------------

void fb_blitline(int x, int y, int width, const uint8_t* pixels)
{
#if GFX_USEFRAMEBUF
    // we use this while blitting from the fb to the lcd - no point in copying back again
    if (!gAllowFramebufUpdate)
        return;

    memcpy(gFramebuf + (y*WIDTH + x), pixels, width);
#endif
}

void fb_readback(int x, int y, int width, int height, col8_t *out_pixels)
{
#if GFX_USEFRAMEBUF

    const int lcd_y_offset = lcd_get_scroll_offset();

    int y_framebuf = (y + lcd_y_offset);
    if (y_framebuf >= BACKBUF_HEIGHT)
        y_framebuf -= BACKBUF_HEIGHT;

    for (int row_ix = 0; row_ix < height; ++row_ix, out_pixels += width)
    {
        memcpy(out_pixels, gFramebuf + (y_framebuf*WIDTH + x), width);

        ++y_framebuf;
        if (y_framebuf >= BACKBUF_HEIGHT)
            y_framebuf -= BACKBUF_HEIGHT;
    }

#endif
}

//----------------------------------------------------------------------------------------

