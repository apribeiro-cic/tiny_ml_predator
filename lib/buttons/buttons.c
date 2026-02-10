#include "buttons.h"

volatile int8_t offset = 0;
volatile bool lock_humidity = false;

// Variável para guardar o tempo do último evento
static volatile uint32_t last_time = 0;

void init_buttons() {
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
}

void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (current_time - last_time < 200) {
        return; 
    }

    last_time = current_time;

    if (gpio == BTN_A) {
        offset -= 5;
    } else if (gpio == BTN_B) {
        offset += 5;
    }

    if (offset > 0 || offset < 0) {
        lock_humidity = true;
        printf("Offset ajustado para %d. Lock de umidade ativado.\n", offset);
    } else {
        lock_humidity = false;
        printf("Offset resetado para 0. Lock de umidade desativado.\n");
    }
}