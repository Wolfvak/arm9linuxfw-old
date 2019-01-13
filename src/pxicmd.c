#include <common.h>
#include <interrupt.h>
#include <ringbuffer.h>
#include <pxicmd.h>

#include <arm/cpu.h>
#include <hw/irq.h>
#include <hw/pxi.h>

#include <hw/prng.h>
#include <hw/sdmmc.h>

#define PXICMD_MAX_DRV  4
#define PXICMD_MAX_JOBS 64

static const pxi_device *drivers[PXICMD_MAX_DRV];
static int driver_count = 0;

int sdmmc_register_driver(void);

DECLARE_RINGBUFFER(pxicmd_jobs, PXICMD_MAX_JOBS);

static int
pxicmd_run_drv(pxi_command *cmd)
{
    const pxi_device *drv;
    const pxi_device_function *func;

    if (cmd->dev < driver_count)
        drv = drivers[cmd->dev];
    else
        return -1;

    if (cmd->function < drv->fn_count)
        func = &drv->functions[cmd->function];
    else
        return -1;

    return (func->pxi_cb)(cmd, drv);
}

static int
pxicmd_handler(u32 unused)
{
    /*
     * takes any incoming commands and puts them on a job ringbuffer
     *
     * interrupt handlers run with interrupts disabled, so there's
     * no need to enter a critical section to access the ring buffer
     */
    while(!pxi_recvfifoempty()) {
        pxi_command *cmd = (pxi_command*)pxi_recv();

        if (ringbuffer_full(pxicmd_jobs)) {
            /* not enough space in the ringbuffer */
            cmd->state = -1;
        } else {
            /* put the job in the ringbuffer */
            cmd->state = 1;
            ringbuffer_store(pxicmd_jobs, cmd);
        }

        /* send ack, if state < 0 then it should be retried by the client */
        pxi_send(cmd->ret_val);
    }
    return IRQ_HANDLED;
}

static int
pxisync_handler(u32 unused)
{
    /*
     * a simple SYNC handler
     * just reply the bitwise negation
     * and send back a SYNC11 interrupt
     */
    pxi_sendsync(~pxi_recvsync());
    pxi_trigger_sync11();
    return IRQ_HANDLED;
}

static void
pxicmd_dumpdrivers(char *out)
{
    /* blindly assume the buffer is big enough */
    for (int i = 0; i < driver_count; i++) {
        const pxi_device *drv = drivers[i];
        strcat(out, drv->name);
        strcat(out, ":");

        for (int j = 0; j < drv->fn_count; j++) {
            const pxi_device_function *fn = &drv->functions[j];
            strcat(out, fn->name);
            strcat(out, ",");
        }
        strcat(out, "\n");
    }
}

static int
sys_list(pxi_command *cmd, const pxi_device *drv)
{
    char *out = PXI_COMMAND_ARG_GET(cmd, 0, char*);
    pxicmd_dumpdrivers(out);
    return 0;
}

static const pxi_device_function sys_ops[] = {
    {
        .name = "list",
        .pxi_cb = sys_list,
    }
};

static const pxi_device sys_drv = {
    .name = "sys",

    .functions = sys_ops,
    .fn_count = ARRAY_SIZE(sys_ops)
};

void
pxicmd_mainloop(void)
{
    /* reset hardware stuff */
    irq_reset();
    pxi_reset();

    /* register interrupt calls */
    irq_register(IRQ_PXI_RECV_NOT_EMPTY, pxicmd_handler);
    irq_register(IRQ_PXI_SYNC, pxisync_handler);

    /* register all drivers */
    pxicmd_install_drv(&sys_drv);
    //prng_register_driver();
    sdmmc_register_driver();

    /* make sure all memory is sane */
    ringbuffer_init(pxicmd_jobs, PXICMD_MAX_JOBS);

    /* enable interrupts, wait for incoming commands */
    enable_interrupts();

    while(1) {
        wait_for_interrupt();

        while(1) {
            pxi_command *cmd = NULL;
            irqsave_status status;

            status = enter_critical_section();
            if (!ringbuffer_empty(pxicmd_jobs)) {
                cmd = ringbuffer_fetch(pxicmd_jobs);
            }
            leave_critical_section(status);

            if (cmd) {
                pxicmd_run_drv(cmd);
                pxi_send(cmd->ret_val);
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
