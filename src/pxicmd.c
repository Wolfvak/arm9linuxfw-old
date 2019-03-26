#include "common.h"
#include "interrupt.h"
#include "ringbuffer.h"
#include "pxicmd.h"

#include "arm/cpu.h"
#include "hw/irq.h"
#include "hw/pxi.h"

#define PXICMD_MAX_DRV  (16)
#define PXICMD_MAX_JOBS (128)

static DECLARE_RINGBUFFER(pxicmd_jobs, PXICMD_MAX_JOBS);

static const pxi_device *drivers[PXICMD_MAX_DRV];
static int driver_count;

static int
pxicmd_count_fn(pxi_command *cmd, const pxi_device *dev)
{
    size_t argc = PXI_COMMAND_ARGC(cmd);

    if (argc == 0)
        return -1;

    for (u32 i = 0; i < argc; i++)
        PXI_COMMAND_ARG_SET(cmd, i, driver_count);
    return 0;
}

static void
own_strncpy(void *dst, const char *src, size_t n)
{
    char *dest = (char*)dst, c;
    do {
        if (n-- == 0) return;
        c = *(src++);
        *(dest++) = c;
    } while(c != '\0');
}

static int
pxicmd_ident_fn(pxi_command *cmd, const pxi_device *dev)
{
    size_t argc = PXI_COMMAND_ARGC(cmd);

    if (argc == 0)
        return -1;

    for (u32 i = 0; i < argc; i++)
        PXI_COMMAND_ARG_SET(cmd, i, 0);
    own_strncpy(&cmd->args[0], dev->name, argc * sizeof(u32));
    return 0;
}

static int
pxicmd_run_drv(pxi_command *cmd)
{
    const pxi_device *dev;

    if (cmd->dev == 0xFF) {
        return pxicmd_count_fn(cmd, NULL);
    } else if (cmd->dev >= driver_count) {
        return -1;
    }

    dev = drivers[cmd->dev];
    if (cmd->function == 0xFF) {
        return pxicmd_ident_fn(cmd, dev);
    } else if (cmd->function >= dev->opcnt) {
        return -1;
    }

    return (dev->ops[cmd->function])(cmd, dev);
}

static int
pxicmd_rx_handler(u32 unused)
{
    /*
     * takes any incoming commands and puts them on a job ringbuffer
     *
     * interrupt handlers run with interrupts disabled, so there's
     * no need to enter a critical section to access the ring buffer
     */
    while(!pxi_rx_empty()) {
        pxi_command *cmd = (pxi_command*)pxi_rx();

        if (ringbuffer_full(pxicmd_jobs)) {
            // not enough space in the ringbuffer
            cmd->state = -1;
        } else {
            // put the job in the ringbuffer
            cmd->state = 1;
            ringbuffer_store(pxicmd_jobs, cmd);
        }

        // send ack, if state " 0 then it should be retried by the client
        pxi_tx(cmd->ret_val);
    }
    return IRQ_HANDLED;
}

static int
pxicmd_sync_handler(u32 unused)
{
    /*
     * simple SYNC handler
     * just reply the bitwise negation and send back a SYNC11 interrupt
     */
    pxi_tx_sync(~pxi_rx_sync());
    pxi_trigger_sync11();
    return IRQ_HANDLED;
}

static void
pxicmd_install_statics(void)
{
    const pxi_device *dev = PXI_COMMAND_DEVICES_FIRST;
    while(dev < PXI_COMMAND_DEVICES_LAST) {
        pxicmd_install_drv(dev++);
    }
}

void __attribute__((noreturn))
pxicmd_mainloop(void)
{
    // reset hardware stuff
    irq_reset();
    pxi_reset();

    // register interrupt calls
    irq_register(IRQ_PXI_RECV_NOT_EMPTY, pxicmd_rx_handler);
    irq_register(IRQ_PXI_SYNC, pxicmd_sync_handler);

    // initialize software stuff
    driver_count = 0;
    ringbuffer_init(pxicmd_jobs, PXICMD_MAX_JOBS);

    // register all drivers
    pxicmd_install_statics();

    enable_interrupts();

    while(1) {
        wait_for_interrupt();

        while(1) {
            pxi_command *cmd;

            ATOMIC_BLOCK {
                cmd = ringbuffer_empty(pxicmd_jobs) ?
                    NULL : ringbuffer_fetch(pxicmd_jobs);
            }

            if (cmd) {
                pxicmd_run_drv(cmd);
                pxi_tx(cmd->ret_val);
            } else {
                break;
            }
        }
    }
}

int
pxicmd_install_drv(const pxi_device *drv)
{
    int ret;
    if (driver_count == PXICMD_MAX_DRV)
        return -1;

    ret = driver_count;
    drivers[driver_count++] = drv;
    return ret;
}
