#include <common.h>
#include <pxicmd.h>

#define REG_PRNG (*(vu32*)0x10011000)

static int
prng_getrandom(pxi_command *cmd, const pxi_device *drv)
{
	for (int i = 0; i < PXI_COMMAND_ARGC(cmd); i++)
		PXI_COMMAND_ARG_SET(cmd, i, REG_PRNG);
	return 0;
}

static const pxi_cb prng_ops[] = {
	prng_getrandom,
};

PXI_COMMAND_DEVICE(prng_device, "prng", prng_ops);
