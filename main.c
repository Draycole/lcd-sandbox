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

    char line[22];
    for (int r = 0; r < 5; r++) {
        for (int i = 0; i < 21; i++) {
            int code = FONT_FIRST_CHAR + r * 21 + i;
            line[i] = (code <= FONT_LAST_CHAR) ? code : ' ';
        }
        line[21] = '\0';
        draw_string(1, 2 + r * 10, line, COLOR_WHITE, COLOR_BLACK, 1);
}
    draw_string(0, 62, "This text is longer than one line and will wrap\nNew line here", COLOR_GREEN, COLOR_BLACK, 1);
    draw_string(0, 100, "Big text", COLOR_RED, COLOR_BLACK, 2);

}