#ifndef TEST_PICO_STDLIB_H
#define TEST_PICO_STDLIB_H
#include <stdbool.h>
#include <stdint.h>
#define __not_in_flash_func(name) name
#define GPIO_IN 0
#define GPIO_OUT 1
/* The stubbed GPIO calls are external, so they already act as barriers here.
 * That also hides a missing barrier from run.py: see check_arm.py. */
#define __compiler_memory_barrier() ((void)0)
void gpio_init(unsigned int pin);
void gpio_set_dir(unsigned int pin, bool output);
void gpio_put(unsigned int pin, bool value);
void gpio_put_masked(uint32_t mask, uint32_t value);
void gpio_init_mask(uint32_t mask);
void gpio_set_dir_out_masked(uint32_t mask);
void gpio_set_dir_in_masked(uint32_t mask);
bool gpio_get(unsigned int pin);
uint32_t gpio_get_all(void);
void sleep_us(uint64_t delay);
#endif
