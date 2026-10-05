A bare-metal C driver for ST7735 (128x160, SPI) TFT displays on the Raspberry Pi Pico (RP2040), written against the Pico SDK with no display library underneath it.

The project is as much about measuring the driver as writing it: every optimization below has a before/after number, and results are reported against the SPI bus's theoretical limit, not just against my own earlier code.