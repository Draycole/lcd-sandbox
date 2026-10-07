#include "st7735.h"
#include "pico/stdlib.h"
#include "font5x7.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

// --- Low-level helpers ---

// Pull CS low to begin talking to the display
static inline void cs_select(void) {
    gpio_put(PIN_CS, 0);
}

// Pull CS high to stop talking to the display
static inline void cs_deselect(void) {
    gpio_put(PIN_CS, 1);
}

// DC low = we are sending a COMMAND byte
static inline void dc_command(void) {
    gpio_put(PIN_DC, 0);
}

// DC high = we are sending DATA bytes (parameters)
static inline void dc_data(void) {
    gpio_put(PIN_DC, 1);
}

// Send a single command byte
static void write_command(uint8_t cmd) {
    cs_select();
    dc_command();
    spi_write_blocking(SPI_PORT, &cmd, 1);
    cs_deselect();
}

// Send a single data byte
static void write_data(uint8_t data) {
    cs_select();
    dc_data();
    spi_write_blocking(SPI_PORT, &data, 1);
    cs_deselect();
}

// --- Hardware reset ---
static void st7735_reset(void) {
    gpio_put(PIN_RES, 1);
    sleep_ms(10);
    gpio_put(PIN_RES, 0);  // Pull reset LOW
    sleep_ms(10);
    gpio_put(PIN_RES, 1);  // Release reset HIGH
    sleep_ms(150);          // Wait for display to recover
}

// new inuput for second grade optimization
static void write_data_buf(const uint8_t *data, size_t len) {
    cs_select();
    dc_data();
    spi_write_blocking(SPI_PORT, data, len);
    cs_deselect();
}


static void st7735_set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t col[4] = {0x00, x0, 0x00, x1};
    uint8_t row[4] = {0x00, y0, 0x00, y1};

    write_command(ST7735_CASET);
    write_data_buf(col, 4);
    write_command(ST7735_RASET);
    write_data_buf(row, 4);
    write_command(ST7735_RAMWR);
}

uint32_t st7735_set_baudrate(uint32_t hz) {
    return spi_set_baudrate(SPI_PORT, hz);   // returns the rate actually achieved
}

/*
// Set the pixel write window to a rectangle region
static void st7735_set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    write_command(ST7735_CASET);  // Column address set
    write_data(0x00);
    write_data(x0);               // Start column
    write_data(0x00);
    write_data(x1);               // End column

    write_command(ST7735_RASET);  // Row address set
    write_data(0x00);
    write_data(y0);               // Start row
    write_data(0x00);
    write_data(y1);               // End row

    write_command(ST7735_RAMWR);  // Now ready to receive pixel data
}
*/

// Start streaming 16-bit pixels (call after set_window)
static inline void pixels_begin(void) {
    spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    cs_select();
    dc_data();
}

// Finish streaming and return to 8-bit frames for commands
static inline void pixels_end(void) {
    cs_deselect();
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

// ---- Original (unoptimized) implementation, kept only for benchmarks ----
static void st7735_set_window_slow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    write_command(ST7735_CASET);
    write_data(0x00);
    write_data(x0);
    write_data(0x00);
    write_data(x1);

    write_command(ST7735_RASET);
    write_data(0x00);
    write_data(y0);
    write_data(0x00);
    write_data(y1);

    write_command(ST7735_RAMWR);
}

static void draw_pixel_slow(uint8_t x, uint8_t y, uint16_t color) {
    st7735_set_window_slow(x, y, x, y);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    cs_select();
    dc_data();
    spi_write_blocking(SPI_PORT, &hi, 1);
    spi_write_blocking(SPI_PORT, &lo, 1);
    cs_deselect();
}

void draw_rect_slow(uint8_t x0, uint8_t y0, uint8_t len, uint8_t wid, uint16_t color) {
    for (uint8_t y = y0; y < y0 + wid; y++) {
        for (uint8_t x = x0; x < x0 + len; x++) {
            draw_pixel_slow(x, y, color);
        }
    }
}

/* used this to time 16bt tray
void st7735_fill_screen_16(uint16_t color){
    st7735_set_window(0, 0, 127, 159);
    uint16_t line[128];                 // one row, one 16-bit value per pixel
    for (int i = 0; i < 128; i++) line[i] = color;

    spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    cs_select();
    dc_data();
    for (int row = 0; row < 160; row++) {
        spi_write16_blocking(SPI_PORT, line, 128);
    }
    cs_deselect();
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);  // back to 8-bit for commands
}    

clearing out for the second optimization
void st7735_fill_screen(uint16_t color){
    st7735_set_window(0, 0, 127, 159);

    //for one row of pixels that is 128px x 2 bytes = 256 bytes
    uint8_t line[128*2];
    for (int i=0; i<128; i++){
        line[2 * i] = color >> 8;       //high byte
        line[2 * i + 1] = color & 0xFF; // low byte. recall we're using rgb 565
    }

    cs_select();
    dc_data();
    for (int row = 0; row < 160; row++){
        spi_write_blocking(SPI_PORT, line, sizeof(line));
    }

    cs_deselect();
}
*/
void st7735_fill_screen_slow(uint16_t color) {
    // ST7735 is 128x160 pixels
    st7735_set_window_slow(0, 0, 127, 159);

    // Each pixel is 2 bytes (16-bit RGB565)
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    cs_select();
    dc_data();
    for (uint32_t i = 0; i < 128 * 160; i++) {
        spi_write_blocking(SPI_PORT, &hi, 1);
        spi_write_blocking(SPI_PORT, &lo, 1);
    }
    cs_deselect();
}

void draw_pixel(uint8_t x, uint8_t y, uint16_t color){
    st7735_set_window(x, y, x, y);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;    

    cs_select();
    dc_data();

    spi_write_blocking(SPI_PORT, &hi, 1);
    spi_write_blocking(SPI_PORT, &lo, 1);

    cs_deselect();
}

//character drawing from the currently number only 0-9 personal library 
void draw_char_slow(uint8_t x, uint8_t y, char c, uint16_t fg, uint16_t bg) {
    uint8_t index = c - FONT_FIRST_CHAR;  // ascii for glyphs listen in font5x7.c

    for (int row = 0; row < 7; row++) {
        uint8_t bits = font5x7[index][row];
        for (int col = 0; col < 5; col++) {
            // shift right, isolate each bit from MSB to LSB
            uint16_t color = (bits >> (4 - col)) & 1 ? fg : bg;
            draw_pixel_slow(x + col, y + row, color);
        }
    }
}

void draw_char_scaled_slow(uint8_t x, uint8_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
    uint8_t index = c - FONT_FIRST_CHAR;

    for (int row = 0; row < 7; row++) {
        uint8_t bits = font5x7[index][row];
        for (int col = 0; col < 5; col++) {
            uint16_t color = (bits >> (4 - col)) & 1 ? fg : bg;
            draw_rect_slow(x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

/* 
//added this newly to see if i can draw circles
void draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint16_t color) {
    for (int x = 0; x < 128; x++) {
        for (int y = 0; y < 160; y++) {
            int dx = x - cx;
            int dy = y - cy;
            if (dx*dx + dy*dy <= r*r) {
                draw_pixel(x, y, color);
            }
        }
    }
}
*/

void draw_filled_circle(uint8_t cx, uint8_t cy, uint8_t r, uint16_t color) {
    // Calculate the bounding box boundaries safely
    int start_x = (cx - r < 0) ? 0 : cx - r;
    int end_x   = (cx + r >= 128) ? 127 : cx + r;
    int start_y = (cy - r < 0) ? 0 : cy - r;
    int end_y   = (cy + r >= 160) ? 159 : cy + r;

    for (int x = start_x; x <= end_x; x++) {
        for (int y = start_y; y <= end_y; y++) {
            int dx = x - cx;
            int dy = y - cy;
            if (dx*dx + dy*dy <= r*r) {
                draw_pixel(x, y, color);
            }
        }
    }
}

void draw_rect(uint8_t x0, uint8_t y0, uint8_t len, uint8_t wid, uint16_t color) {
    if (len == 0 || wid == 0 || x0 >= 128 || y0 >= 160) return;
    if (x0 + len > 128) len = 128 - x0;      // clip to the screen
    if (y0 + wid > 160) wid = 160 - y0;

    st7735_set_window(x0, y0, x0 + len - 1, y0 + wid - 1);

    uint16_t row[128];
    for (int i = 0; i < len; i++) row[i] = color;

    pixels_begin();
    for (int y = 0; y < wid; y++) {
        spi_write16_blocking(SPI_PORT, row, len);
    }
    pixels_end();
}

void st7735_fill_screen(uint16_t color) {
    draw_rect(0, 0, 128, 160, color);        // one code path for both
}

#define MAX_SCALE 6

void draw_char_scaled(uint8_t x, uint8_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
    if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) return;           // font only has digits for now
    if (scale == 0 || scale > MAX_SCALE) return;

    const int w = 5 * scale;
    const int h = 7 * scale;
    if (x + w > 128 || y + h > 160) return;   // simple rule: don't draw if it won't fit

    static uint16_t buf[(5 * MAX_SCALE) * (7 * MAX_SCALE)];
    const uint8_t *glyph = font5x7[c - FONT_FIRST_CHAR];

    // Build the glyph's pixels in RAM, each font pixel becoming scale x scale pixels
    for (int row = 0; row < 7; row++) {
        for (int sy = 0; sy < scale; sy++) {
            for (int col = 0; col < 5; col++) {
                uint16_t color = ((glyph[row] >> (4 - col)) & 1) ? fg : bg;
                for (int sx = 0; sx < scale; sx++) {
                    buf[(row * scale + sy) * w + col * scale + sx] = color;
                }
            }
        }
    }

    st7735_set_window(x, y, x + w - 1, y + h - 1);
    pixels_begin();
    spi_write16_blocking(SPI_PORT, buf, w * h);
    pixels_end();
}

void draw_char(uint8_t x, uint8_t y, char c, uint16_t fg, uint16_t bg) {
    draw_char_scaled(x, y, c, fg, bg, 1);
}

// Draws one line of text (no wrapping): n characters starting at s.
// The caller guarantees the line fits within the screen width.
static void draw_text_line(uint8_t x, uint8_t y, const char *s, int n,
                           uint16_t fg, uint16_t bg, uint8_t scale) {
    const int w = n * FONT_ADVANCE * scale;
    const int h = FONT_HEIGHT * scale;
    uint16_t row_buf[128];

    st7735_set_window(x, y, x + w - 1, y + h - 1);   // one window for the whole line
    pixels_begin();

    for (int row = 0; row < FONT_HEIGHT; row++) {
        // Build one pixel row across all the characters
        int px = 0;
        for (int i = 0; i < n; i++) {
            char c = s[i];
            if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) c = '?';
            uint8_t bits = font5x7[c - FONT_FIRST_CHAR][row];

            for (int col = 0; col < FONT_ADVANCE; col++) {
                // columns 0-4 are the glyph, column 5 is the gap
                uint16_t color = (col < FONT_WIDTH && ((bits >> (4 - col)) & 1)) ? fg : bg;
                for (int sx = 0; sx < scale; sx++) row_buf[px++] = color;
            }
        }
        // Send that row 'scale' times (this is the vertical scaling)
        for (int sy = 0; sy < scale; sy++) {
            spi_write16_blocking(SPI_PORT, row_buf, w);
        }
    }
    pixels_end();
}

void draw_string(uint8_t x, uint8_t y, const char *str,
                 uint16_t fg, uint16_t bg, uint8_t scale) {
    if (scale == 0 || scale > MAX_SCALE || x >= 128) return;

    const int adv = FONT_ADVANCE * scale;
    const int max_chars = (128 - x) / adv;       // characters that fit on one line
    if (max_chars == 0) return;
    const int line_h = (FONT_HEIGHT + 1) * scale;  // 7 rows of glyph + 1 row of spacing

    int cy = y;
    while (*str) {
        if (cy + FONT_HEIGHT * scale > 160) return;   // no room for another line

        int n = 0;                                    // characters on this line
        while (str[n] && str[n] != '\n' && n < max_chars) n++;

        if (n > 0) draw_text_line(x, cy, str, n, fg, bg, scale);

        str += n;
        if (*str == '\n') str++;                      // skip the newline itself
        cy += line_h;
    }
}

// --- Initialisation sequence ---
void st7735_init(void) {

    // 1. Initialise SPI peripheral at 8MHz, Mode 0
    spi_init(SPI_PORT, 8000000);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // 2. Assign SPI function to the hardware pins
    gpio_set_function(18, GPIO_FUNC_SPI);  // SCK
    gpio_set_function(19, GPIO_FUNC_SPI);  // MOSI

    // 3. Set DC, RES, CS as regular GPIO outputs
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_RES);
    gpio_set_dir(PIN_RES, GPIO_OUT);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);

    cs_deselect();  // Start with CS high (display not selected)

    // 4. Hardware reset
    st7735_reset();

    // 5. Command sequence — wake the display up
    write_command(ST7735_SWRESET);  // Software reset
    sleep_ms(150);

    write_command(ST7735_SLPOUT);   // Exit sleep mode
    sleep_ms(500);                  // Datasheet requires 500ms after SLPOUT

    write_command(ST7735_COLMOD);   // Set color format
    write_data(0x05);               // 0x05 = 16-bit RGB565 color

    write_command(ST7735_MADCTL);   // Memory access control (orientation)
    write_data(0xC0);               // Default orientation for now | corrected to accoutn for orientation mixup

    write_command(ST7735_NORON);    // Normal display mode
    sleep_ms(10);

    write_command(ST7735_DISPON);   // Turn display on
    sleep_ms(100);
}