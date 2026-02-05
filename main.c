
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/status_led.h"

#include "drivers/picocalc.h"
#include "drivers/lcd.h"
#include "drivers/keyboard.h"

#include "libcalc/libcalc.h"


bool power_off_requested = false;


void set_onboard_led(uint8_t led)
{
    status_led_set_state(led & 0x01);
}


void init_platform()
{
    status_led_init();

    stdio_init_all();
    picocalc_init();
}


void str_to_lower(char *s) {
    while (*s) {
        *s = tolower((unsigned char)*s);
        s++;
    }
}


void readline(char *buffer, size_t size)
{
    size_t index = 0;
    for (;;)
    {
        char ch = getchar();
        if (ch == '\n' || ch == '\r')
        {
            printf("\n");
            break; // End of line
        }
        else if ((ch == KEY_BACKSPACE || ch == 0x7f) && index > 0)
        {
            index--;
            buffer[index] = '\0'; // Remove last character
            printf("\b \b"); // Erase the last character
        }
        else if (ch >= 0x20 && ch < 0x7F && index < size - 1) // Printable characters
        {
            buffer[index++] = ch;
            putchar(ch);
        }
    }
    buffer[index] = '\0'; // Null-terminate the string
}


//-------------------------------------------------------------------------------------------------

static bool cmd_bye(const char*)
{
    rom_reset_usb_boot(0, 0);
    return true;
}

static bool cmd_big(const char*)
{
    lcd_set_font(&font_10x16);
    return true;
}

static bool cmd_small(const char*)
{
    lcd_set_font(&font_5x10);
    return true;
}

//-------------------------------------------------------------------------------------------------


int main()
{
    char inputBuf[256];
    char outputBuf[512];

    init_platform();

    calc_init(lcd_emit_str);
    register_calc_cmd(cmd_big, "big", "", "switches to big text");
    register_calc_cmd(cmd_small, "small", "", "switches to small text");
    register_calc_cmd(cmd_bye, "bye", "", "resets to BOOTSEL");

    printf(MCALC_WELCOME);

    for (;;)
    {
        printf("\n>");

        readline(inputBuf, sizeof(inputBuf));
        if (strlen(inputBuf) == 0)
            continue; // Skip empty input

        reset_plot();

        lcd_erase_cursor();
        
        calc_eval(inputBuf, outputBuf, sizeof(outputBuf));
        puts(outputBuf);

        const Plot* plot = get_plot();
        if (plot)
        {
            lcd_put_image(plot->Pixels, MC_PLOT_WIDTH, MC_PLOT_HEIGHT);
        }
    }
}
