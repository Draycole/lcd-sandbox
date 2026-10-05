#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "st7735.h"
 
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

    while (1) {
        printf("-- slow vs fast --\n");
        printf("fill_screen:       %llu -> %llu us (%.1fx)\n", (unsigned long long)fill_s,   (unsigned long long)fill_f,   (double)fill_s / fill_f);
        printf("draw_char:         %llu -> %llu us (%.1fx)\n", (unsigned long long)char_s,   (unsigned long long)char_f,   (double)char_s / char_f);
        printf("draw_char_scaled3: %llu -> %llu us (%.1fx)\n", (unsigned long long)scaled_s, (unsigned long long)scaled_f, (double)scaled_s / scaled_f);
        printf("draw_rect 30x30:   %llu -> %llu us (%.1fx)\n", (unsigned long long)rect_s,   (unsigned long long)rect_f,   (double)rect_s / rect_f);
        sleep_ms(3000);
    }
}