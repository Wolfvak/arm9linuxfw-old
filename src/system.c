#include "common.h"

#include "pxicmd.h"

static int
system_fwver(pxi_command *cmd, const pxi_device *dev)
{
	for (int i = 0; i < PXI_COMMAND_ARGC(cmd); i++)
		PXI_COMMAND_ARG_SET(cmd, i, FW_VER);
	return 0;
}

static const pxi_cb system_ops[] = {
    system_fwver,
};

PXI_COMMAND_DEVICE(system_device, "system", system_ops);
