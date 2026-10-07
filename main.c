#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "st7735.h"
#include "font5x7.h"

#define N 20

// return avg time in us after running statement N times
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

    sleep_ms(1500);
    st7735_fill_screen(COLOR_RED);

    sleep_ms(1000);
    draw_rect(2, 5, 20, 20, COLOR_WHITE);
    draw_string(10, 50, "Hello mi amor", COLOR_WHITE, COLOR_RED, 1);
    draw_char_scaled(40, 100, 'W', COLOR_WHITE, COLOR_RED, 5);
    draw_filled_circle(50, 79, 8, COLOR_GREEN);

    while (1) {

    }
}
