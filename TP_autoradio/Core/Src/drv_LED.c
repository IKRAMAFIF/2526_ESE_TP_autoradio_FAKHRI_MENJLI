
#include"drv_LED.h"
#include <stdint.h>
#include "main.h"
extern SPI_HandleTypeDef hspi3;
extern LED_Driver_t led_driver;

void write_MCP23017(uint8_t registre, uint8_t value)
{
	uint8_t commande[3];
	commande[0]=0x40; //
	commande[1]=registre;
	commande[2]=value;

	HAL_GPIO_WritePin(GPIOA, VU_nRESET_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, VU_nCS_Pin, GPIO_PIN_RESET); //SC=0 pour l'envoie
	HAL_SPI_Transmit(&hspi3, commande, 3, HAL_MAX_DELAY);
	HAL_Delay(10);
	HAL_GPIO_WritePin(GPIOB,VU_nCS_Pin, GPIO_PIN_SET);



}

uint8_t read_MCP23S17(uint8_t reg) {
	uint8_t data[3];

	data[0] = 0x41;
	data[1] = reg;
	data[2] = 0x00;

	HAL_GPIO_WritePin(GPIOB, VU_nCS_Pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi3, data, data, 3, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(GPIOB, VU_nCS_Pin, GPIO_PIN_SET);

	return data[2];
}

void init_MCP23017(void)
{
	write_MCP23017(0x00,0x00); // les oins A en sortie
	write_MCP23017(0x01,0x00); // les pins B en sortie
	write_MCP23017(0x12,0xFF);
	write_MCP23017(0x13,0x0FF);

}
void Blink_LEDs(void) {
	while (1) {
		write_MCP23017(0x13, 0xFF);
		write_MCP23017(0x12, 0xFF);
		HAL_Delay(300);
		write_MCP23017(0x13, 0x00);
		write_MCP23017(0x12, 0x00);
		HAL_Delay(300);
	}
}
void LED_Chenillard(void) {
	for (uint8_t i = 0; i < 8; i++) {
		write_MCP23017(0x13, (1 << i));
		write_MCP23017(0x12, (1 << i));
		HAL_Delay(300);
	}

}

void LED_Driver_Init(LED_Driver_t *driver) {

driver->init = init_MCP23017;
driver->write = write_MCP23017;
driver->read = read_MCP23S17;
driver->chenillard = LED_Chenillard;
driver->blink_all = Blink_LEDs;
driver->init();
}

int shell_control_led(h_shell_t *h_shell, int argc, char **argv) {

	if (argc != 4) {
		snprintf(h_shell->print_buffer, BUFFER_SIZE, "Usage: l <port> <pin> <state>\r\n");
		h_shell->drv.transmit(h_shell->print_buffer, strlen(h_shell->print_buffer));
		return -1;
	}


	char port = argv[1][0];
	int pin = atoi(argv[2]);
	int state = atoi(argv[3]);


	if ((port != 'A' && port != 'B') || pin < 0 || pin > 7 || (state != 0 && state != 1)) {
		snprintf(h_shell->print_buffer, BUFFER_SIZE, "Invalid arguments\r\n");
		h_shell->drv.transmit(h_shell->print_buffer, strlen(h_shell->print_buffer));
		return -1;
	}


	uint8_t reg = (port == 'A') ? 0x13 : 0x12;
	uint8_t current_state = 0;


	if (led_driver.read) {
		current_state = led_driver.read(reg);
	}


	if (state == 1) {
		current_state &= ~(1 << pin);
	} else {
		current_state |= (1 << pin);
	}

	led_driver.write(reg, current_state);


	snprintf(h_shell->print_buffer, BUFFER_SIZE, "LED %c%d %s\r\n", port, pin, state ? "ON" : "OFF");
	h_shell->drv.transmit(h_shell->print_buffer, strlen(h_shell->print_buffer));

	return 0;
}
