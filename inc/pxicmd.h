#pragma once

#include <common.h>
#include <hw/pxi.h>

#define PXI_COMMAND_ARG_GET(cmd, argn, type)	((type)((cmd)->args[argn]))
#define PXI_COMMAND_ARG_SET(cmd, argn, value)	((cmd)->args[argn] = (u32)(value))

typedef struct {
	u8 dev;
	u8 function;

	s8 state;
	s8 argc;
	u32 args[0];
} __attribute__((packed)) pxi_command;

typedef struct pxi_device pxi_device;

typedef struct pxi_device_function {
	/* function name */
	const char *name;

	/* function entrypoint */
	int (*pxi_cb)(pxi_command *cmd, const pxi_device *dev);
} pxi_device_function;

typedef struct pxi_device {
	/* device name */
	const char *name;

	/* array of device operations & count */
	const pxi_device_function *functions;
	int fn_count;
} pxi_device;

int pxicmd_install_drv(const pxi_device *drv);
