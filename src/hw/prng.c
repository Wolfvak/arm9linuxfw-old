#include <common.h>
#include <pxicmd.h>

#include <hw/prng.h>

#define REG_PRNG (*(vu32*)0x10011000)

static int
prng_getrandom(pxi_command *cmd, const pxi_device *drv)
{
	for (int i = 0; i < cmd->argc; i++)
		PXI_COMMAND_ARG_SET(cmd, i, REG_PRNG);
	return 0;
}

static const pxi_device_function prng_functions[] = {
	{
		.name = "rand",
		.pxi_cb = prng_getrandom,
	},
};

static const pxi_device prng_device = {
	.name = "prng",

	.functions = prng_functions,
	.fn_count = ARRAY_SIZE(prng_functions),
};

int
prng_register_driver(void)
{
	return pxicmd_install_drv(&prng_device);
}
