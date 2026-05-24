//
//  PicoCalc LCD display driver
//
//  This driver interfaces with the ST7789P LCD controller on the PicoCalc.
//
//  It is optimised for a character-based display with a fixed-width, 8-pixel wide font
//  and 65K colours in the RGB565 format. This driver requires little memory as it
//  uses the frame memory on the controller directly.
//
//  NOTE: Some code below is written to respect timing constraints of the ST7789P controller.
//        For instance, you can usually get away with a short chip select high pulse widths, but
//        writing to the display RAM requires the minimum chip select high pulse width of 40ns.
//

#include <cstdio>
#include <cstring>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/spi.h"

#include "lcd.h"


#define LCD_SPI         (spi1)          // SPI interface for the LCD display

// Raspberry Pi Pico board GPIO pins
#define LCD_SCL         (10)            // serial clock (SCL)
#define LCD_SDI         (11)            // serial data in (SDI) from mcu -> lcd. aka MISO, SDA, DIN
#define LCD_SDO         (12)            // serial data out (SDO) from lcd -> mcu. aka MOSI, DOUT
#define LCD_CSX         (13)            // chip select (CSX)
#define LCD_DCX         (14)            // data/command (D/CX)
#define LCD_RST         (15)            // reset (RESET)


// LCD interface definitions
// According to the ST7789P datasheet, the maximum SPI clock speed is 62.5 MHz.
// However, the controller can handle 75 MHz in practice.
#define LCD_BAUDRATE    (75*1000*1000)  // 75 MHz SPI clock speed
#define LCD_READRATE    (12*1000*1000)  // slower SPI speed for reading. things get unstable as this gets faster

// LCD command definitions
#define LCD_CMD_NOP     (0x00)          // no operation
#define LCD_CMD_SWRESET (0x01)          // software reset
#define LCD_CMD_RDID    (0x04)          // Read Display ID
#define LCD_CMD_RDDST   (0x09)          // Read Display Status
#define LCD_CMD_SLPIN   (0x10)          // sleep in
#define LCD_CMD_SLPOUT  (0x11)          // sleep out
#define LCD_CMD_INVOFF  (0x20)          // display inversion off
#define LCD_CMD_INVON   (0x21)          // display inversion on
#define LCD_CMD_DISPOFF (0x28)          // display off
#define LCD_CMD_DISPON  (0x29)          // display on
#define LCD_CMD_COLADDRSET   (0x2A)     // column address set
#define LCD_CMD_ROWADDRSET   (0x2B)     // row address set
#define LCD_CMD_RAMWR   (0x2C)          // memory write
#define LCD_CMD_RAMRD   (0x2E)          // memory read
#define LCD_CMD_VSCRDEF (0x33)          // vertical scroll definition
#define LCD_CMD_MADCTL  (0x36)          // memory access control
#define LCD_CMD_VSCSAD  (0x37)          // vertical scroll start address of RAM
#define LCD_CMD_COLMOD  (0x3A)          // pixel format set
#define LCD_CMD_IFMODE  (0xB0)          // interface mode control
#define LCD_CMD_FRMCTR1 (0xB1)          // frame rate control (in normal mode)
#define LCD_CMD_FRMCTR2 (0xB2)          // frame rate control (in idle mode)
#define LCD_CMD_FRMCTR3 (0xB3)          // frame rate control (in partial mode)
#define LCD_CMD_DIC     (0xB4)          // display inversion control
#define LCD_CMD_DFC     (0xB6)          // display function control
#define LCD_CMD_EMS     (0xB7)          // entry mode set
#define LCD_CMD_MODESEL (0xB9)          // mode set
#define LCD_CMD_PWR1    (0xC0)          // power control 1
#define LCD_CMD_PWR2    (0xC1)          // power control 2
#define LCD_CMD_PWR3    (0xC2)          // power control 3
#define LCD_CMD_VCMPCTL (0xC5)          // VCOM control
#define LCD_CMD_RDID1   (0xDA)          // read ID1
#define LCD_CMD_RDID2   (0xDB)          // read ID2
#define LCD_CMD_RDID3   (0xDC)          // read ID3
#define LCD_CMD_PGC     (0xE0)          // positive gamma control
#define LCD_CMD_NGC     (0xE1)          // negative gamma control
#define LCD_CMD_DGC1    (0xE2)          // driver gamma control 1
#define LCD_CMD_DGC2    (0xE3)          // driver gamma control
#define LCD_CMD_DOCA    (0xE8)          // driver output control
#define LCD_CMD_E9      (0xE9)          // Manufacturer command
#define LCD_CMD_F0      (0xF0)          // Manufacturer command
#define LCD_CMD_F7      (0xF7)          // Manufacturer command

#define FRAME_HEIGHT    (480)           // frame memory height in pixels



// Display control functions
void lcd_reset(void);
void lcd_display_on(void);
void lcd_display_off(void);

// Low-level SPI functions
void lcd_write_cmd(uint8_t cmd);
void lcd_write_data(uint8_t len, ...);
void lcd_write16_data(uint8_t len, ...);
void lcd_write16_buf(const uint16_t *buffer, size_t len);

uint8_t lcd_cmd_read1(uint8_t cmd);



static bool lcd_initialised = false; // flag to indicate if the LCD is initialised

// TODO: reinstate support for frozen top/bottom areas
//static uint16_t lcd_scroll_top = 0;                      // top fixed area for vertical scrolling
//static uint16_t lcd_memory_scroll_height = FRAME_HEIGHT; // scroll area height
//static uint16_t lcd_scroll_bottom = 0;                   // bottom fixed area for vertical scrolling
static uint16_t lcd_y_offset = 0;                        // offset for vertical scrolling

// Background processing
static uint32_t irq_state;

static void lcd_disable_interrupts()
{
    irq_state = save_and_disable_interrupts();
    //gpio_put(3, true);
}

static void lcd_enable_interrupts()
{
    //gpio_put(3, false);
    restore_interrupts(irq_state);
}

//
// Low-level SPI functions
//

// Send a command
void lcd_write_cmd(uint8_t cmd)
{
    gpio_put(LCD_DCX, 0); // Command
    gpio_put(LCD_CSX, 0);
    spi_write_blocking(LCD_SPI, &cmd, 1);
    gpio_put(LCD_CSX, 1);
}

// Send 8-bit data (byte)
void lcd_write_data(uint8_t len, ...)
{
    va_list args;
    va_start(args, len);
    gpio_put(LCD_DCX, 1); // Data
    gpio_put(LCD_CSX, 0);
    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t data = va_arg(args, int); // get the next byte of data
        spi_write_blocking(LCD_SPI, &data, 1);
    }
    gpio_put(LCD_CSX, 1);
    va_end(args);
}

// Send 16-bit data (half-word)
void lcd_write16_data(uint8_t len, ...)
{
    va_list args;

    // DO NOT MOVE THE spi_set_format() OR THE gpio_put(LCD_DCX) CALLS!
    // They are placed before the gpio_put(LCD_CSX) to ensure that a minimum
    // chip select high pulse width is achieved (at least 40ns)
    spi_set_format(LCD_SPI, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    va_start(args, len);
    gpio_put(LCD_DCX, 1); // Data
    gpio_put(LCD_CSX, 0);
    for (uint8_t i = 0; i < len; i++)
    {
        uint16_t data = va_arg(args, int); // get the next half-word of data
        spi_write16_blocking(LCD_SPI, &data, 1);
    }
    gpio_put(LCD_CSX, 1);
    va_end(args);

    spi_set_format(LCD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

void lcd_write16_buf(const uint16_t *buffer, size_t len)
{
    // DO NOT MOVE THE spi_set_format() OR THE gpio_put(LCD_DCX) CALLS!
    // They are placed before the gpio_put(LCD_CSX) to ensure that a minimum
    // chip select high pulse width is achieved (at least 40ns)
    spi_set_format(LCD_SPI, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_put(LCD_DCX, 1); // Data
    gpio_put(LCD_CSX, 0);
    spi_write16_blocking(LCD_SPI, buffer, len);
    gpio_put(LCD_CSX, 1);

    spi_set_format(LCD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

uint8_t lcd_cmd_read1(uint8_t cmd)
{
    gpio_put(LCD_CSX, 0);
    gpio_put(LCD_DCX, 0);
    spi_write_blocking(LCD_SPI, &cmd, 1);

    gpio_put(LCD_DCX, 1);

    const uint8_t dummyTx = 0xff;
    uint8_t dummyRx = -1;

    spi_read_blocking(LCD_SPI, dummyTx, &dummyRx, 1);

    uint8_t out = 0xbb;
    spi_read_blocking(LCD_SPI, dummyTx, &out, 1);

    gpio_put(LCD_CSX, 1);

    return out;
}


//
//  ST7365P LCD controller functions
//

// Select the target of the pixel data in the display RAM that will follow
static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // Set column address (X)
    lcd_write_cmd(LCD_CMD_COLADDRSET);
    lcd_write_data(4,
                   UPPER8(x0), LOWER8(x0),
                   UPPER8(x1), LOWER8(x1));

    // Set row address (Y)
    lcd_write_cmd(LCD_CMD_ROWADDRSET);
    lcd_write_data(4,
                   UPPER8(y0), LOWER8(y0),
                   UPPER8(y1), LOWER8(y1));
}

//
//  Send pixel data to the display
//
//  All display RAM updates come through this function. This function is responsible for
//  setting the correct window in the display RAM and writing the pixel data to it. It also
//  handles the vertical scrolling by adjusting the y-coordinate based on the current scroll
//  offset (lcd_y_offset).
//
//  The pixel data is expected to be in RGB565 format, which is a 16-bit value with the
//  red component in the upper 5 bits, the green component in the middle 6 bits, and the
//  blue component in the lower 5 bits.

void lcd_blit(const uint16_t *pixels, int x, int y, int width, int height)
{
    lcd_disable_interrupts();

    // TODO: reinstate support for frozen top & bottom areas
    
    int y_framebuf = (y + lcd_y_offset);
    if (y_framebuf >= FRAME_HEIGHT)
        y_framebuf -= FRAME_HEIGHT;

    // cope with dest area wrapping around end of frame buf
    const int overflow = (y_framebuf + height) - FRAME_HEIGHT;
    if (overflow <= 0)
    {
        lcd_set_window(x, y_framebuf, x + width - 1, y_framebuf + height - 1);

        lcd_write_cmd(LCD_CMD_RAMWR);
        lcd_write16_buf(pixels, width * height);
    }
    else
    {
        const uint16_t height_btm = overflow;
        const uint16_t height_top = height - overflow;

        lcd_set_window(x, y_framebuf, x + width - 1, FRAME_HEIGHT - 1);
        lcd_write_cmd(LCD_CMD_RAMWR);
        lcd_write16_buf(pixels, width * height_top);

        const uint16_t* pixels_btm = pixels + (width * height_top);
        lcd_set_window(x, 0, x + width - 1, height_btm - 1);
        lcd_write_cmd(LCD_CMD_RAMWR);
        lcd_write16_buf(pixels_btm, width * height_btm);
    }

    lcd_enable_interrupts();
}

// Draw a solid rectangle on the display
void lcd_rect(int x, int y, int width, int height, uint16_t col)
{
    static uint16_t pixels[WIDTH];

    for (uint16_t i = 0; i < width; i++)
    {
        pixels[i] = col;
    }
    for (uint16_t row = 0; row < height; row++)
    {
        lcd_blit(pixels, x, y + row, width, 1);
    }
}


void lcd_readback(int x, int y, int width, int height, uint16_t *out_pixels)
{
    lcd_disable_interrupts();
    
    int y_framebuf = (y + lcd_y_offset);
    if (y_framebuf >= FRAME_HEIGHT)
        y_framebuf -= FRAME_HEIGHT;

    lcd_set_window(x, y_framebuf, x + width - 1, y_framebuf + height - 1);
    
    spi_set_baudrate(LCD_SPI, LCD_READRATE);

    gpio_put(LCD_CSX, 0);
    gpio_put(LCD_DCX, 0);
    const uint8_t cmd = LCD_CMD_RAMRD;
    spi_write_blocking(LCD_SPI, &cmd, 1);

    gpio_put(LCD_DCX, 1);

    for (int i=0; i<10000; ++i)
        __nop();

    const uint8_t dummyTx = 0;
    uint8_t dummyRx = -1;

    spi_read_blocking(LCD_SPI, dummyTx, &dummyRx, 1);

    spi_set_format(LCD_SPI, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    for (int i=0; i<10000; ++i)
        __nop();

    spi_read16_blocking(LCD_SPI, dummyTx, out_pixels, width * height);

    gpio_put(LCD_CSX, 1);

    spi_set_baudrate(LCD_SPI, LCD_BAUDRATE);
    spi_set_format(LCD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    lcd_enable_interrupts();
}

//
//  Scrolling area of the display
//
//  This forum post provides a good explanation of how scrolling on the ST7789P display works:
//      https://forum.arduino.cc/t/st7735s-scrolling/564506
//
//  These functions (lcd_define_scrolling, lcd_scroll_up, and lcd_scroll_down) configure and
//  set the vertical scrolling area of the display, but it is the responsibility of lcd_blit()
//  to ensure that the pixel data is written to the correct location in the display RAM.
//

void lcd_define_scrolling(uint16_t top_fixed_area, uint16_t bottom_fixed_area)
{
    // TODO: reinstate support for frozen top/bottom areas
    top_fixed_area = 0;
    bottom_fixed_area = 0;

    // uint16_t scroll_area = HEIGHT - (top_fixed_area + bottom_fixed_area);
    // if (scroll_area == 0 || scroll_area > FRAME_HEIGHT)
    // {
    //     // Invalid scrolling area, reset to full screen
    //     top_fixed_area = 0;
    //     bottom_fixed_area = 0;
    //     scroll_area = FRAME_HEIGHT;
    // }
    
    // lcd_scroll_top = top_fixed_area;
    // lcd_memory_scroll_height = FRAME_HEIGHT - (top_fixed_area + bottom_fixed_area);
    // lcd_scroll_bottom = bottom_fixed_area;

    lcd_disable_interrupts();
    lcd_write_cmd(LCD_CMD_VSCRDEF);
    lcd_write_data(6,
                   UPPER8(top_fixed_area),
                   LOWER8(top_fixed_area),
                   UPPER8(FRAME_HEIGHT),
                   LOWER8(FRAME_HEIGHT),
                   UPPER8(bottom_fixed_area),
                   LOWER8(bottom_fixed_area));
    lcd_enable_interrupts();

    lcd_scroll_reset(); // Reset the scroll area to the top
}

void lcd_scroll_reset()
{
    // Clear the scrolling area by filling it with the background colour
    lcd_y_offset = 0; // Reset the scroll offset
    uint16_t scroll_area_start = lcd_y_offset;

    lcd_disable_interrupts();
    lcd_write_cmd(LCD_CMD_VSCSAD); // Sets where in display RAM the scroll area starts
    lcd_write_data(2, UPPER8(scroll_area_start), LOWER8(scroll_area_start));
    lcd_enable_interrupts();
}

void lcd_scroll_clear(uint16_t col)
{
    lcd_scroll_reset(); // Reset the scroll area to the top

    // Clear the scrolling area
    lcd_rect(0, 0, WIDTH, HEIGHT, col);
}

// Scroll the screen up (make space at the bottom)
void lcd_scroll_up(uint32_t distance, uint16_t clearCol)
{
    // This will rotate the content in the scroll area up by one line
    lcd_y_offset = (lcd_y_offset + distance) % FRAME_HEIGHT;
    uint16_t scroll_area_start = lcd_y_offset;

    lcd_disable_interrupts();
    lcd_write_cmd(LCD_CMD_VSCSAD); // Sets where in display RAM the scroll area starts
    lcd_write_data(2, UPPER8(scroll_area_start), LOWER8(scroll_area_start));
    lcd_enable_interrupts();

    // Clear the new line at the bottom
    lcd_rect(0, HEIGHT - distance, WIDTH, distance, clearCol);
}

// Scroll the screen down one line (making space at the top)
void lcd_scroll_down(uint32_t distance, uint16_t clearCol)
{
    // This will rotate the content in the scroll area down by one line
    lcd_y_offset = (lcd_y_offset - distance + FRAME_HEIGHT) % FRAME_HEIGHT;
    uint16_t scroll_area_start = lcd_y_offset;

    lcd_disable_interrupts();
    lcd_write_cmd(LCD_CMD_VSCSAD); // Sets where in display RAM the scroll area starts
    lcd_write_data(2, UPPER8(scroll_area_start), LOWER8(scroll_area_start));
    lcd_enable_interrupts();

    // Clear the new line at the top
    lcd_rect(0, 0, WIDTH, distance, clearCol);
}

//
// Text drawing functions
//

// Clear the entire screen
void lcd_clear_screen(uint16_t col)
{
    lcd_scroll_reset(); // Reset the scrolling area to the top
    lcd_rect(0, 0, WIDTH, FRAME_HEIGHT, col);
}
 

//
//  Display control functions
//

// Reset the LCD display
void lcd_reset()
{
    gpio_put(LCD_CSX, 1);

    // Blip the reset pin to reset the LCD controller
    gpio_put(LCD_RST, 1);
    busy_wait_us(20);

    gpio_put(LCD_RST, 0);
    busy_wait_us(20); // 20µs reset pulse (10µs minimum)

    gpio_put(LCD_RST, 1);
    busy_wait_us(120000); // 5ms required after reset, but 120ms needed before sleep out command
}

// Turn on the LCD display
void lcd_display_on()
{
    lcd_disable_interrupts();
    lcd_write_cmd(LCD_CMD_DISPON);
    lcd_enable_interrupts();
}

// Turn off the LCD display
void lcd_display_off()
{
    lcd_disable_interrupts();
    lcd_write_cmd(LCD_CMD_DISPOFF);
    lcd_enable_interrupts();
}


// Initialize the LCD display
bool lcd_init(uint16_t clearCol)
{
    if (lcd_initialised)
        return true; // already initialized

    // initialise GPIO
    gpio_init(LCD_SCL);
    gpio_init(LCD_SDI);
    gpio_init(LCD_SDO);
    gpio_init(LCD_CSX);
    gpio_init(LCD_DCX);
    gpio_init(LCD_RST);

    gpio_set_dir(LCD_SCL, GPIO_OUT);
    gpio_set_dir(LCD_SDI, GPIO_OUT);
    gpio_set_dir(LCD_CSX, GPIO_OUT);
    gpio_set_dir(LCD_DCX, GPIO_OUT);
    gpio_set_dir(LCD_RST, GPIO_OUT);

    // initialise 4-wire SPI
    spi_init(LCD_SPI, LCD_BAUDRATE);
    gpio_set_function(LCD_SCL, GPIO_FUNC_SPI);
    gpio_set_function(LCD_SDI, GPIO_FUNC_SPI);
    gpio_set_function(LCD_SDO, GPIO_FUNC_SPI);
    gpio_set_input_hysteresis_enabled(LCD_SDO, true);

    gpio_put(LCD_CSX, 1);
    gpio_put(LCD_RST, 1);

    lcd_disable_interrupts();

    lcd_reset(); // reset the LCD controller
  
    lcd_write_cmd(LCD_CMD_SWRESET); // reset the commands and parameters to their S/W Reset default values
    busy_wait_us(10000);                   // required to wait at least 5ms

#if 0
    // extra setup cribbed from cw demo lcdspi.c
    lcd_write_cmd(LCD_CMD_PGC);
    lcd_write_data(15,
        0x00, 0x03, 0x09, 0x08,
        0x16, 0x0a, 0x3f, 0x78,
        0x4c, 0x09, 0x0a, 0x08,
        0x16, 0x1a,
        0x0f);  // NOTE: ST7365P docs only specify 14 params... shrug

    lcd_write_cmd(LCD_CMD_NGC);
    lcd_write_data(15,
        0x00, 0x16, 0x19, 0x03,
        0x0F, 0x05, 0x32, 0x45,
        0x46, 0x04, 0x0E, 0x0D,
        0x35, 0x37, 0x0F);
    
    lcd_write_cmd(LCD_CMD_PWR1);
    lcd_write_data(2, 0x17, 0x15);
    lcd_write_cmd(LCD_CMD_PWR2);
    lcd_write_data(1, 0x41);

    lcd_write_cmd(LCD_CMD_VCMPCTL);
    lcd_write_data(3, 0x00, 0x12, 0x80);

    lcd_write_cmd(LCD_CMD_IFMODE);
    lcd_write_data(1, 0x00);

    lcd_write_cmd(LCD_CMD_FRMCTR1);
    lcd_write_data(2, 0xA0, 0x00);

    lcd_write_cmd(LCD_CMD_DIC);
    lcd_write_data(1, 0x02);

    lcd_write_cmd(LCD_CMD_DFC);
    lcd_write_data(3, 0x02, 0x02, 0x3B);
#endif

    lcd_write_cmd(LCD_CMD_COLMOD); // pixel format set
    lcd_write_data(1, 0x55);       // 16 bit/pixel (RGB565)

    lcd_write_cmd(LCD_CMD_MADCTL); // memory access control
    lcd_write_data(1, 0x48);       // BGR colour filter panel, top to bottom, left to right

    lcd_write_cmd(LCD_CMD_INVON); // display inversion on

    lcd_write_cmd(LCD_CMD_EMS); // entry mode set
    lcd_write_data(1, 0x46);    // normal display, 16-bit (RGB) to 18-bit (rgb) colour
                                //   conversion: r, b msb => lsb

    lcd_write_cmd(LCD_CMD_VSCRDEF); // vertical scroll definition
    lcd_write_data(6,
                   0x00, 0x00, // top fixed area of 0 pixels
                   0x01, 0x40, // scroll area height of 320 pixels
                   0x00, 0x00  // bottom fixed area of 0 pixels
    );

    lcd_write_cmd(LCD_CMD_SLPOUT); // sleep out
    lcd_enable_interrupts();

    busy_wait_us(10000);                  // required to wait at least 5ms

    // Clear the screen
    lcd_clear_screen(clearCol);

    #if 0
    // debug alignment check
    //uint16_t stripes[] = { 0xffff, 0x0000, 0xa0a0, 0x0a0a, 0xc0c0, 0xacac, 0xf800, 0x0018 };
    //   actual              ffff    0000    d0d0    0505    e0e0    d6d6    00fc    0c00
    //uint16_t stripes[] = { 0xff00, 0x00ff, 0x0f0f, 0x0102, 0x0408, 0xf800, 0x07e0, 0x0018 };
    //   actual              00ff    ff00    0707    0100    0402    00fc    f003    0c00
    uint16_t stripes[] = { 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x0000, 0x0000, 0x0000 };
    static_assert(sizeof(stripes) == 16);
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; x += 8)
        {
            lcd_blit(stripes, x, y, 8, 1);
        }
    }
    #endif

    // Now that the display is initialized, display RAM garbage is cleared,
    // turn on the display
    lcd_display_on();

    lcd_initialised = true; // Set the initialised flag
    return true;
}