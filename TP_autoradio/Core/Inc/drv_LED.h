#ifndef INC_drv_LED_H_
#define INC_drv_LED_H_

#include <stdint.h>
#include "shell.h"
typedef struct {
void (*init)(void);
void (*write)(uint8_t reg, uint8_t value);
void (*test_first_led)(void);
void (*chenillard)(void);
void (*blink_all)(void);
uint8_t (*read)(uint8_t reg);
} LED_Driver_t;

void write_MCP23017(uint8_t reg, uint8_t value);
void init_MCP23017(void);
void Blink_LEDs(void);
void LED_Chenillard(void);
int shell_control_led(h_shell_t *h_shell, int argc, char **argv);
#endif
