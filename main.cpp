
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
#include "drivers/picocalc.h"
#include "drivers/text.h"

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


void readline()
{
    for (;;)
    {
        const char c = getchar();
        input_process_char(c);

        if (input_has_complete_line())
            return;
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

//-------------------------------------------------------------------------------------------------


int main()
{
    char outputBuf[1024];

    init_platform();

    calc_init(text_emit_str);
    register_calc_cmd(cmd_big, "big", "", "switches to big text");
    register_calc_cmd(cmd_small, "small", "", "switches to small text");
    register_calc_cmd(cmd_bye, "bye", "", "resets to BOOTSEL");

    text_emit_str(MCALC_WELCOME);
    input_reset_line();

    for (;;)
    {
        readline();

        cursor_erase();
        
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
