#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

#define SPI_PORT spi1
#define SPI_PIN_SCK 10 // GP10
#define SPI_PIN_MOSI 11 // GP11
#define REFV 2.5 // Using a TL431 in it's default state
#define SPI_CLK_SPEED (1000 * 1000) // 1Mhz
                                    //
#define ADC_SPI_PORT spi1
#define ADC_PIN_CS 9 // GP9

int main(void)
{
    spi_init(SPI_PORT, SPI_CLK_SPEED);
    spi_set_format(SPI_PORT, MCP4921_WRITE_LEN, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(mcp->clk_pin, GPIO_FUNC_SPI);
    gpio_set_function(mcp->mosi_pin, GPIO_FUNC_SPI);

    gpio_init(mcp->cs_pin);
    gpio_set_dir(mcp->cs_pin, GPIO_OUT);
    gpio_put(mcp->cs_pin, 1);

    return 0;
}
