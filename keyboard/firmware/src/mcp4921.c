#include <math.h>
#include <stdio.h>
#include "mcp4921.h"

static inline void cs_select(struct mcp4921 *mcp)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(mcp->cs_pin, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(struct mcp4921 *mcp)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(mcp->cs_pin, 1);  // Active low
    asm volatile("nop \n nop \n nop");
}

int mcp4921_init(struct mcp4921* mcp)
{
    spi_init(mcp->spi_inst, mcp->clock_speed);
    spi_set_format(mcp->spi_inst, MCP4921_WRITE_LEN, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(mcp->clk_pin, GPIO_FUNC_SPI);
    gpio_set_function(mcp->mosi_pin, GPIO_FUNC_SPI);

    gpio_init(mcp->cs_pin);
    gpio_set_dir(mcp->cs_pin, GPIO_OUT);
    gpio_put(mcp->cs_pin, 1);

    return 0;
}

int mcp4921_set_output(struct mcp4921 *dac, float volts)
{
    unsigned int gain = mcp4921_get_gain(dac) == MCP4921_GAIN_1X ? 1 : 2;
    float dac_value = floor((MCP4921_MAX_VAL * volts) / (dac->refv * gain));

    uint16_t dac_out = (dac->cmd_flags << 12) | (uint16_t) dac_value;
    cs_select(dac);
    int bytes_written = spi_write16_blocking(
        dac->spi_inst,
        &dac_out,
        1
    );
    cs_deselect(dac);

    return 0;
}
