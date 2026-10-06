/* need to run a clock sweep.
saving previous main

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "st7735.h"
#include "font5x7.h"

#define N 20

// Runs a statement N times and gives the average time in microseconds
#define TIME_AVG_US(n, ...) ({                       \
    uint64_t _t0 = time_us_64();                     \
    for (int _i = 0; _i < (n); _i++) { __VA_ARGS__; }\
    (time_us_64() - _t0) / (n);                      \
})

int main(void) {
    stdio_init_all();
    sleep_ms(2000); 

    st7735_init();
    st7735_fill_screen(COLOR_BLACK);

    uint64_t fill_s   = TIME_AVG_US(5, st7735_fill_screen_slow(COLOR_BLUE));
    uint64_t fill_f   = TIME_AVG_US(5, st7735_fill_screen(COLOR_BLUE));
    uint64_t char_s   = TIME_AVG_US(N, draw_char_slow(30, 20, '8', COLOR_WHITE, COLOR_BLACK));
    uint64_t char_f   = TIME_AVG_US(N, draw_char(30, 20, '8', COLOR_WHITE, COLOR_BLACK));
    uint64_t scaled_s = TIME_AVG_US(N, draw_char_scaled_slow(60, 20, '8', COLOR_WHITE, COLOR_BLACK, 3));
    uint64_t scaled_f = TIME_AVG_US(N, draw_char_scaled(60, 20, '8', COLOR_WHITE, COLOR_BLACK, 3));
    uint64_t rect_s   = TIME_AVG_US(N, draw_rect_slow(10, 80, 30, 30, COLOR_RED));
    uint64_t rect_f   = TIME_AVG_US(N, draw_rect(10, 80, 30, 30, COLOR_RED));
    
    const char *msg = "ST7735 driver on Pico";   // 21 characters
    uint64_t str_slow = TIME_AVG_US(5, for (int i = 0; i < 21; i++) draw_char_slow(1 + i * 6, 20, msg[i], COLOR_WHITE, COLOR_BLACK));
    uint64_t str_char = TIME_AVG_US(N, for (int i = 0; i < 21; i++) draw_char(1 + i * 6, 20, msg[i], COLOR_WHITE, COLOR_BLACK));
    uint64_t str_fast = TIME_AVG_US(N, draw_string(1, 20, msg, COLOR_WHITE, COLOR_BLACK, 1));

    while (1) {
        printf("-- slow vs fast --\n");
        printf("fill_screen:       %llu -> %llu us (%.1fx)\n", (unsigned long long)fill_s,   (unsigned long long)fill_f,   (double)fill_s / fill_f);
        printf("draw_char:         %llu -> %llu us (%.1fx)\n", (unsigned long long)char_s,   (unsigned long long)char_f,   (double)char_s / char_f);
        printf("draw_char_scaled3: %llu -> %llu us (%.1fx)\n", (unsigned long long)scaled_s, (unsigned long long)scaled_f, (double)scaled_s / scaled_f);
        printf("draw_rect 30x30:   %llu -> %llu us (%.1fx)\n", (unsigned long long)rect_s,   (unsigned long long)rect_f,   (double)rect_s / rect_f);

        uint32_t baud = spi_get_baudrate(SPI_PORT);
        double ideal_str = (11.0 + 2.0 * 882) * 8.0 / baud * 1e6;   // 882 = 21 chars x 6 x 7 px
        printf("string (21 chars): per-pixel %llu us | draw_char loop %llu us | draw_string %llu us\n",
               (unsigned long long)str_slow, (unsigned long long)str_char, (unsigned long long)str_fast);
        
        printf("draw_string: %.0f chars/sec, ideal %.0f us, %.1f%% efficient\n",
               21e6 / str_fast, ideal_str, 100.0 * ideal_str / str_fast);
        sleep_ms(3000);
    }
}
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "st7735.h"
#include "font5x7.h"

#define TIME_AVG_US(n, ...) ({                       \
    uint64_t _t0 = time_us_64();                     \
    for (int _i = 0; _i < (n); _i++) { __VA_ARGS__; }\
    (time_us_64() - _t0) / (n);                      \
})

#define NSPEEDS 5

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    uint32_t requests[NSPEEDS] = {8000000, 16000000, 21000000, 32000000, 63000000};
    uint32_t actual[NSPEEDS];
    uint64_t fill_us[NSPEEDS];

    for (int i = 0; i < NSPEEDS; i++) {
        st7735_init();                                   // known-good state at the default 8 MHz
        actual[i] = st7735_set_baudrate(requests[i]);    // now raise the clock

        fill_us[i] = TIME_AVG_US(5, st7735_fill_screen(COLOR_BLUE));

        // Visual check: colour bars plus a label showing the actual clock
        draw_rect(0,  0, 32, 160, COLOR_RED);
        draw_rect(32, 0, 32, 160, COLOR_GREEN);
        draw_rect(64, 0, 32, 160, COLOR_BLUE);
        draw_rect(96, 0, 32, 160, COLOR_WHITE);

        char label[24];
        snprintf(label, sizeof label, "%.2f MHz", actual[i] / 1e6);
        draw_string(4, 70, label, COLOR_WHITE, COLOR_BLACK, 2);
        sleep_ms(4000);                                  // time to look at the screen
    }

    while (1) {
        printf("-- clock sweep --\n");
        for (int i = 0; i < NSPEEDS; i++) {
            double ideal = (11.0 + 2.0 * 128 * 160) * 8.0 / actual[i] * 1e6;
            printf("req %lu -> actual %lu Hz: fill %llu us (ideal %.0f us, %.1f%%)\n",
                   (unsigned long)requests[i], (unsigned long)actual[i],
                   (unsigned long long)fill_us[i], ideal, 100.0 * ideal / fill_us[i]);
        }
        sleep_ms(3000);
    }
}