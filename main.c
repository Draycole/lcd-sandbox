//clock sweep test
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
        st7735_init();                                  
        actual[i] = st7735_set_baudrate(requests[i]);   

        fill_us[i] = TIME_AVG_US(5, st7735_fill_screen(COLOR_BLUE));

        // run a visual check to make sure colours printed match 
        draw_rect(0,  0, 32, 160, COLOR_RED);
        draw_rect(32, 0, 32, 160, COLOR_GREEN);
        draw_rect(64, 0, 32, 160, COLOR_BLUE);
        draw_rect(96, 0, 32, 160, COLOR_WHITE);

        char label[24];
        snprintf(label, sizeof label, "%.2f MHz", actual[i] / 1e6);
        draw_string(4, 70, label, COLOR_WHITE, COLOR_BLACK, 2);
        sleep_ms(4000);                                  // time to check the screen
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