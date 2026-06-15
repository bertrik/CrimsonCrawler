#include <stdint.h>

typedef void (vsync_fn_t) (int frame_nr);

void led_init(vsync_fn_t * vsync);
void led_write_framebuffer(uint8_t data[8][32]);

void led_enable(void);
void led_disable(void);
void led_divider(uint32_t divider);
void led_set_gamma(const uint8_t *data);

void IRAM_ATTR matrix_cmd(uint8_t cmd, uint8_t data);
void IRAM_ATTR matrix_load(bool level);
void IRAM_ATTR matrix_shift(uint8_t data);
