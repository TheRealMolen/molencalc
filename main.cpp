
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/status_led.h"

#include "drivers/font.h"
#include "drivers/lcd.h"
#include "drivers/keyboard.h"
#include "drivers/palette.h"
#include "drivers/picocalc.h"
#include "drivers/text.h"

#include "libcalc/libcalc.h"


bool power_off_requested = false;


void set_onboard_led(uint8_t led)
{
    status_led_set_state(led & 0x01);
}


void init_platform(const Palette* palette)
{
    status_led_init();

    stdio_init_all();

    gfx_set_palette(palette);
    picocalc_init(text_get_background());
}


void str_to_lower(char *s) {
    while (*s) {
        *s = tolower((unsigned char)*s);
        s++;
    }
}

//-------------------------------------------------------------------------------------------------

static bool cmd_bye(const char*)
{
    rom_reset_usb_boot(0, 0);
    return true;
}

static bool cmd_big(const char*)
{
    text_set_font(&font_10x16);
    return true;
}

static bool cmd_small(const char*)
{
    text_set_font(&font_5x10);
    return true;
}

static bool cmd_screenshot(const char*)
{
    col8_t pixels[WIDTH*HEIGHT];

    //lcd_readback(0, 0, WIDTH, HEIGHT, pixels);
    fb_readback(0, 0, WIDTH, HEIGHT, pixels);

    const Palette* pal = gfx_get_palette();
    if (!pal)
    {
        text_emit_str("\nerr: no palette\n");
        return false;
    }
    constexpr int numCols = 256;
    uint8_t palRgb[numCols * 4];
    pal->ExportAsRGBQuads(palRgb, numCols);

   /* {
        char head[1024];
        sprintf(head, "data starts: %04x %04x %04x %04x\n"
                      "             %04x %04x %04x %04x\n",
            int(pixels[0]), int(pixels[1]), int(pixels[2]), int(pixels[3]),
            int(pixels[4]), int(pixels[5]), int(pixels[6]), int(pixels[7]));

        text_emit_str(head);
    }*/

    FILE* fp = fopen("scr.data", "wb");
    if (!fp)
    {
        text_emit_str("\nerr: unable to open screenshot file for writing\n");
        return false;
    }

    size_t total_bytes = sizeof(pixels);
    const char* outbuf = reinterpret_cast<char*>(pixels);
    size_t total_written = 0;
    for (;;)
    {
        size_t bytes_written = fwrite(outbuf, 1, total_bytes - total_written, fp);
        total_written += bytes_written;
        outbuf += bytes_written;
        if (total_written == total_bytes)
            break;

        if (bytes_written == 0)
        {
            text_emit_str("Img write failed. Abandoning.\n");
            break;
        }

        text_emit_str("write to img continuing...\n");
    }

    fwrite(palRgb, 1, sizeof(palRgb), fp);

    fclose(fp);

    return true;
}

//-------------------------------------------------------------------------------------------------

void log_to_settings(const char* str)
{
    FILE* log_fp = fopen("mc.log", "a");
    if (!log_fp)
    {
        text_emit_str("\nerr: unable to open log for writing\n");
        return;
    }

    size_t total_bytes = strlen(str);
    size_t total_written = 0;
    for (;;)
    {
        size_t bytes_written = fwrite(str + total_written, 1, total_bytes - total_written, log_fp);
        total_written += bytes_written;
        if (total_written == total_bytes)
        {
            fwrite("\n", 1, 1, log_fp);
            break;
        }

        if (bytes_written == 0)
        {
            text_emit_str("Log write failed\n");
            break;
        }

        text_emit_str("Warning: write to log needed another go...\n");
    }

    fclose(log_fp);
}

//-------------------------------------------------------------------------------------------------

void readline()
{
    for (;;)
    {
        const char c = getchar();

        if (c == KEY_F1)
        {
            cmd_bye(nullptr);
            continue;
        }
        else if (c == KEY_F5)
        {
            cmd_screenshot(nullptr);
            input_reset_line();
            continue;
        }

        input_process_char(c);

        if (input_has_complete_line())
            return;
    }
}

//-------------------------------------------------------------------------------------------------

int main()
{
    char outputBuf[1024];

    init_platform(palette_get_dark());

    calc_init(text_emit_str);
    register_calc_cmd(cmd_big, "big", "", "switches to big text");
    register_calc_cmd(cmd_small, "small", "", "switches to small text");
    register_calc_cmd(cmd_bye, "bye", "", "resets to BOOTSEL");
    register_calc_cmd(cmd_screenshot, "scr", "", "save a screenshot");

    text_emit_str(MCALC_WELCOME);
    input_reset_line();

    for (;;)
    {
        readline();

        cursor_erase();

        log_to_settings(input_get_line());
        
        reset_plot();
        calc_eval(input_get_line(), outputBuf, sizeof(outputBuf));
        text_emit_str(outputBuf);

        if (const Plot* plot = get_plot())
        {
            cursor_erase();
            text_put_image(plot->Pixels, MC_PLOT_WIDTH, MC_PLOT_HEIGHT);
        }

        input_reset_line();
    }
}
