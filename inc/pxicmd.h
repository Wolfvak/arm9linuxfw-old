#pragma once

#include <common.h>
#include <hw/pxi.h>

/*
 * PXI is a hardware block contained in the 3DS system, which both the
 * ARM11 and ARM9 cores can access.
 *
 * It's essentially a 16-word bidirectional FIFO with interrupt capability
 * (RX is not empty, TX is full, etc).
 *
 * The higher level protocol consists of a very simple structure containing
 * the following data:
 *
 * @ret_val: this value is to be sent through PXI back to the calling processor.
 * Currently, it consists of the virtual address of the command header in the
 * ARM11 address space, but it could be anything (like an index, a bitmask, etc)
 *
 * @dev: the device that you're trying to call
 * @function: the subfunction within the device
 * @state: the acknowledgement state:
 *  < 0 => command was received but can't be processed (invalid dev, etc)
 *  > 0 => command was received and is being processed - expect a "done" signal
 *  = 0 => command was not received (timed out)
 *
 * @argc: argument count, there's currently a hard limit of 256 arguments
 * The following data contains the arguments, each one is a u32.
 *
 * When a command is received by the ARM9 it sets the state to the appropriate
 * value and sends back an ACK signal.
 *
 * If the command can't be processed, the ARM11 must try again whenever
 * there is a chance (after some time or when the next command is done).
 *
 * If the command is being processed, the ARM9 will send another command when
 * it's finished - the ARM11 must know whether this is the first (ACK) or
 * second (DONE) time it's been sent.
 */

#define PXI_COMMAND_ARG_GET(cmd, argn, type)	((type)((cmd)->args[argn]))
#define PXI_COMMAND_ARG_SET(cmd, argn, value)	((cmd)->args[argn] = (u32)(value))

typedef struct {
	u32 ret_val;

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
