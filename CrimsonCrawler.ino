#include <Arduino.h>

#include <stdint.h>

#include <MiniShell.h>

#include "leddriver.h"

#define print Serial.printf

#define LED_HEIGHT 8
#define LED_WIDTH  32

static MiniShell shell(&Serial);
static uint8_t framebuffer[LED_HEIGHT][LED_WIDTH];
static volatile uint32_t frame_counter = 0;

static int do_pix(int argc, char *argv[])
{
    if (argc < 4) {
        return -1;
    }
    int x = atoi(argv[1]);
    int y = atoi(argv[2]);
    uint8_t c = atoi(argv[3]);
    if ((x >= 32) || (y >= 8)) {
        return -1;
    }
    framebuffer[y][x] = c;
    return 0;
}

static void draw_fill(uint8_t c)
{
    memset(framebuffer, c, sizeof(framebuffer));
}

static int do_pat(int argc, char *argv[])
{
    if (argc < 2) {
        return -1;
    }
    int pat = atoi(argv[1]);

    switch (pat) {
    case 0:
        print("All clear\n");
        draw_fill(0);
        break;
    case 1:
        print("All on\n");
        draw_fill(255);
        break;
    case 2:
        print("Gradient 1\n");
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 32; x++) {
                framebuffer[y][x] = x * x / 4;
            }
        }
        break;
    case 3:
        print("Gradient 2\n");
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 32; x++) {
                framebuffer[y][x] = y * y * 4;
            }
        }
        break;
    default:
        print("Unhandled pattern %d\n", pat);
        return -1;
    }

    return 0;
}

static int do_fps(int argc, char *argv[])
{
    print("Measuring ...");
    uint32_t count = frame_counter;
    delay(1000);
    int fps = frame_counter - count;
    print("FPS = %d\n", fps);
    return 0;
}

static int do_enable(int argc, char *argv[])
{
    bool enable = true;
    if (argc > 1) {
        enable = atoi(argv[1]) != 0;
    }
    if (enable) {
        led_enable();
    } else {
        led_disable();
    }
    return 0;
}

static int do_reboot(int argc, char *argv[])
{
    led_disable();
    ESP.restart();
    return 0;
}

static int do_init(int argc, char *argv[])
{
    // scan limit
    matrix_load(0);
    for (int i = 0; i < 4; i++) {
        matrix_shift(0x0b);
        matrix_shift(7);
    }
    matrix_load(1);
    delay(1);

    // shutdown
    matrix_load(0);
    for (int i = 0; i < 4; i++) {
        matrix_shift(0x0C);
        matrix_shift(1);
    }
    matrix_load(1);

    return 0;
}

static int do_bright(int argc, char *argv[])
{
    if (argc < 2) {
        return -1;
    }
    int intens = atoi(argv[1]);

    // intensity
    matrix_load(0);
    for (int i = 0; i < 4; i++) {
        matrix_shift(0x0A);
        matrix_shift(intens);
    }
    matrix_load(1);
    return 0;
}

static int do_divider(int argc, char *argv[])
{
    if (argc < 2) {
        return -1;
    }
    uint32_t div = strtoul(argv[1], NULL, 0);
    print("Setting divider %d\n", div);
    led_divider(div);
    return 0;
}

static const cmd_t commands[] = {
    { "bright", do_bright, "Brightness" },
    { "init", do_init, "Initialise" },
    { "pix", do_pix, "<col> <row> [intensity] Set pixel" },
    { "pat", do_pat, "<pattern> Set pattern" },
    { "fps", do_fps, "Show FPS" },
    { "enable", do_enable, "[0|1] Enable/disable" },
    { "reboot", do_reboot, "Reboot" },
    { "div", do_divider, "Set divider" },
    { NULL, NULL, NULL }
};

// vsync callback
static void IRAM_ATTR vsync(int frame_nr)
{
    led_write_framebuffer(framebuffer);
    frame_counter = frame_nr;
}

void setup(void)
{
    Serial.begin(115200);
    led_init(vsync);

}

void loop(void)
{
    shell.process(">", commands);
    // network update
    // MDNS.update();
    // ArduinoOTA.handle();
}
