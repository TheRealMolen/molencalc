
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


int main()
{
    char inputBuf[256];
    char outputBuf[512];

    init_platform();

    printf("molencalc v15      don't panic\n");

    for (;;)
    {
        printf("\n>");

        readline(inputBuf, sizeof(inputBuf));
        if (strlen(inputBuf) == 0)
            continue; // Skip empty input

        // 'bye' on the command line reboots to BOOTSEL mode
        if (strcmp(inputBuf, "bye") == 0)
        {
            rom_reset_usb_boot(0, 0);
            return 0;
        }
        else if (strcmp(inputBuf, "big") == 0)
        {
            lcd_set_font(&font_10x16);
            continue;
        }
        else if (strcmp(inputBuf, "small") == 0)
        {
            lcd_set_font(&font_5x10);
            continue;
        }

        calc_eval(inputBuf, outputBuf, sizeof(outputBuf));
        puts(outputBuf);

        const Plot* plot = get_plot();
        if (plot)
        {
            lcd_put_image(plot->Pixels, MC_PLOT_WIDTH, MC_PLOT_HEIGHT);
            reset_plot();
        }
    }
}
