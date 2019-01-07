#pragma once

#include <common.h>
#include <hw/pxi.h>

enum {
	DRV_OK = 0,
	DRV_IO_ERROR,
	DRV_TOO_MANY,
	DRV_NOT_FOUND,
	DRV_BAD_MEMORY,
};

typedef struct {
	u32 dev;
	u32 cmd;
	int state;
	u32 args[5];
} __attribute__((packed)) pxi_cmd;

typedef struct pxicmd_drv pxicmd_drv;

typedef struct pxidrv_fns {
	u32 func_id;
	int (*func_ptr)(pxi_cmd *cmd, pxicmd_drv *drv);
} pxidrv_fns;

typedef struct pxicmd_drv {
	u32 id;
	u32 fn_cnt;
	pxidrv_fns *fns;
	void *priv;
} pxicmd_drv;

int pxicmd_install_drv(pxicmd_drv *drv);
int pxicmd_run_drv(pxi_cmd *cmd);

static inline void
pxicmd_reply(pxi_cmd *cmd) {
	pxi_send((u32)cmd);
}
