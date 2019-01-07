#include <common.h>

#include <arm/cpu.h>
#include <hw/pxi.h>
#include <pxicmd.h>

#include <ringbuffer.h>

#define PXICMD_MAX_DRV 8

static pxicmd_drv *drivers[PXICMD_MAX_DRV];
static u32 driver_count = 0;

DEFINE_RINGBUFFER(pxicmd_jobs, 32);

static inline pxicmd_drv *
pxicmd_find_drv(u32 dev) {
    for (u32 i = 0; i < driver_count; i++) {
        if (dev == drivers[i]->id)
            return drivers[i];
    }
    return NULL;
}

static inline pxidrv_fns *
pxicmd_find_fn(u32 cmd, pxicmd_drv *drv) {
    for (u32 i = 0; i < drv->fn_cnt; i++) {
        if (cmd == drv->fns[i].func_id)
            return &drv->fns[i];
    }
    return NULL;
}

int
pxicmd_install_drv(pxicmd_drv *drv)
{
    int ret;
    if (driver_count == PXICMD_MAX_DRV)
        return -DRV_TOO_MANY;

    ret = driver_count;
    drivers[driver_count++] = drv;
    return ret;
}

int
pxicmd_run_drv(pxi_cmd *cmd)
{
    pxicmd_drv *drv;
    pxidrv_fns *func;

    drv = pxicmd_find_drv(cmd->dev);

    if (drv == NULL)
        return -DRV_NOT_FOUND;

    func = pxicmd_find_fn(cmd->cmd, drv);
    return (func->func_ptr)(cmd, drv);
}

int
pxicmd_handler(u32 irqn)
{
    pxi_cmd *cmd;

    while((cmd = (pxi_cmd*)pxi_recv())) {
        if (ringbuffer_full(&pxicmd_jobs)) {
            cmd->state = -DRV_TOO_MANY;
            pxicmd_reply(cmd);
        } else {
            ringbuffer_store(&pxicmd_jobs, cmd);
        }
    }
    return 0;
}

int
pxisync_handler(u32 irqn)
{
    pxi_sendsync(~pxi_recvsync());
    pxi_trigger_sync11();
    return 0;
}

void
pxicmd_mainloop(void)
{
    enable_interrupts();
    while(1) {
        wait_for_interrupt();

        while(1) {
            pxi_cmd *cmd = NULL;
            irqsave_status status;

            status = enter_critical_section();
            if (!ringbuffer_empty(&pxicmd_jobs)) {
                cmd = ringbuffer_fetch(&pxicmd_jobs);
            }
            leave_critical_section(status);

            if (cmd) {
                pxicmd_run_drv(cmd);
            } else {
                break;
            }
        }
    }
}
