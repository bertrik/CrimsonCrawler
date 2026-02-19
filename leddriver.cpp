#include <string.h>

#include <Arduino.h>

#include "leddriver.h"

#define PIN_DIN     D4
#define PIN_CS       D3
#define PIN_CLK       D2

static const vsync_fn_t *vsync_fn;
static uint8_t framebuffer[8][32];
static uint8_t pwmbuf[8][32];
static int row = 0;
static int frame = 0;

#define FAST_GPIO_WRITE(pin,val) if (val) GPOS = 1<<(pin); else GPOC = 1<<(pin)

void IRAM_ATTR matrix_shift(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        FAST_GPIO_WRITE(PIN_CLK, 0);
        FAST_GPIO_WRITE(PIN_DIN, data & 0x80);
        FAST_GPIO_WRITE(PIN_CLK, 1);
        data = data << 1;
    }
}

void IRAM_ATTR matrix_load(bool level)
{
    FAST_GPIO_WRITE(PIN_CS, level);
}

void IRAM_ATTR matrix_cmd(uint8_t cmd, uint8_t data)
{
    FAST_GPIO_WRITE(PIN_CS, 0);
    matrix_shift(cmd);
    matrix_shift(data);
    FAST_GPIO_WRITE(PIN_CS, 1);
}

// "horizontal" interrupt routine, displays one line
static void IRAM_ATTR led_hsync(void)
{
    FAST_GPIO_WRITE(PIN_CS, 0);

    // write column data
    uint8_t *rowbuf = pwmbuf[row];
    int x = 0;
    for (int m = 0; m < 4; m++) {
        matrix_shift(row + 1);
        for (int col = 0; col < 8; col++) {
            int val = rowbuf[x];
            val += framebuffer[row][x];
            rowbuf[x] = val;
            bool c = val > 255;
            x++;

            // write data
            FAST_GPIO_WRITE(PIN_CLK, 0);
            FAST_GPIO_WRITE(PIN_DIN, c);
            FAST_GPIO_WRITE(PIN_CLK, 1);
        }
    }

    FAST_GPIO_WRITE(PIN_CS, 1);

    // next row
    row = (row + 1) % 8;
    if (row == 0) {
        vsync_fn(frame++);
    }
}

void IRAM_ATTR led_write_framebuffer(const void *data)
{
    memcpy(framebuffer, data, sizeof(framebuffer));
}

void led_init(vsync_fn_t *pfn)
{
    vsync_fn = pfn;

    // set all pins to a defined state
    pinMode(PIN_DIN, OUTPUT);
    digitalWrite(PIN_DIN, 0);
    pinMode(PIN_CS, OUTPUT);
    digitalWrite(PIN_CS, 1);
    pinMode(PIN_CLK, OUTPUT);
    digitalWrite(PIN_CLK, 0);

    // clear the frame buffer
    memset(framebuffer, 0, sizeof(framebuffer));

    // initialise PWM buffer
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 32; x++) {
            pwmbuf[y][x] = random(255);
        }
    }

    // initialise timer
    timer1_isr_init();
}

void led_enable(void)
{
    // set up timer interrupt
    timer1_disable();
    timer1_attachInterrupt(led_hsync);
    timer1_write(1000);         // fps = 555555/number
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
}

void led_divider(uint32_t divider)
{
    timer1_write(divider);
}

void led_disable(void)
{
    // detach the interrupt routine
    timer1_detachInterrupt();
    timer1_disable();
}
