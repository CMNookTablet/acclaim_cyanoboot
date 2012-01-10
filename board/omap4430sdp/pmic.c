#include <common.h>
#include <omap4_hs.h>
#include <i2c.h>

#define PMIC_ACCESS_TIMEOUT (100)

#define CHECK_ERROR( status ) \
	if ( (status) != NO_ERROR ) { return (FAILED); }


extern int select_bus(int bus, int speed);

void pmic_set_vpp(void)
{
	u8 data = 0;

	select_bus(CFG_I2C_BUS, CFG_I2C_SPEED);

	data = 0x1;
	i2c_write(0x48, 0x9c, 1, &data, 1);  // VPP_CFG_GRP

	data = 0x3;
	i2c_write(0x48, 0x9d, 1, &data, 1);  // VPP_CFG_TRANS

	data = 0x21;
	i2c_write(0x48, 0x9e, 1, &data, 1);  // VPP_CFG_STATE

	data = 0x8;
	i2c_write(0x48, 0x9f, 1, &data, 1);  // VPP_CFG_VOLTAGE

	return ;
}

void pmic_close_vpp(void)
{
	u8 data = 0;
	select_bus(CFG_I2C_BUS, CFG_I2C_SPEED);

	data = 0x20;
	i2c_write(0x48, 0x9e, 1, &data, 1);  // VPP_CFG_STATE

	return ;
}
