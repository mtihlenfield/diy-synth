#ifndef __MCP4921_H__
#define __MCP4921_H__
/*
 * Intended to be a somewhat generic library for the MCP4921
 */

#include <stddef.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define MCP4921_MAX_VAL 4095
#define MCP4921_WRITE_LEN 16

#define MCP4921_GAIN_1X 1
#define MCP4921_GAIN_2X 0

#define MCP4921_SHDN_ON 1
#define MCP4921_SHDN_OFF 0

#define MCP4921_VREF_BUFFERED 1
#define MCP4921_VREF_UNBUFFERED 0

#define MCP4921_DAC_A 0
#define MCP4921_DAC_B 1

// TODO document these functions

#define mcp4921_set_dac(mcp, dac) (mcp)->cmd_flags |= ((dac) << 3)
#define mcp4921_set_buff(mcp, buff_state) (mcp)->cmd_flags |= ((buff_state) << 2)
#define mcp4921_set_gain(mcp, gain) (mcp)->cmd_flags |= ((gain) << 1)
#define mcp4921_set_shdn(mcp, shdn_state) (mcp)->cmd_flags |= (shdn_state)

#define mcp4921_get_gain(mcp) ((mcp)->cmd_flags & 0b0100) >> 2

struct mcp4921 {
    uint8_t clk_pin;
    uint8_t cs_pin;
    uint8_t mosi_pin;
    spi_inst_t *spi_inst;
    float refv;
    unsigned int clock_speed;
    uint8_t cmd_flags: 4;
};

int mcp4921_init(struct mcp4921* mcp);

int mcp4921_set_output(struct mcp4921 *dac, float volts);

#endif
