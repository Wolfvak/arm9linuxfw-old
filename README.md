# arm9linuxfw

This repository contains the source to arm9linuxfw, that is, the software that runs on the ARM9 while the 3DS is running Linux on the main processor. It provides access to the MMC controller and potentially any other ARM9-exclusive hardware.

Currently, it also contains the patch to be able to build Linux. Until I figure out proper kernel source management, it'll remain like that.

Most of the Linux code was started by xerpi @ github.com/xerpi/linux_3ds, I just took it, moved it to linux 4.20, added a bunch of extra drivers and wrote my own arm9linuxfw from scratch. You can still use his firm-linux-loader.

# known bugs

tested only on an old 3DS XL!

arm9linuxfw:
- none so far, although I'm sure there's plenty of them, both because of coding mistakes and because of misunderstanding the hardware

Linux:
- reboot doesn't work (doesn't even call the reboot function, needs further investigation)
- RTC doesn't properly work (consistently incorrect, probably just me not knowing how to write Linux drivers)
- when reading too much data the SD driver sometimes hardlocks the entire system (I've tested reading sectors 0-10e6 and those work fine, so I'm assuming it's a Linux driver bug rather than arm9linuxfw's fault)

# TODO

arm9linuxfw:
- proper MMC testing
- bootrom / OTP drivers
- AES driver
- use NDMA on all of these

Linux:
- improvements to the PXI command protocol
- add proper autodiscovery to the PXI "bus"
- wireless communications
- drivers for all of the above
- GPU (KMS only to start with, of course)
- DSP
- some standard rootfs built specifically for the 3DS
- new 3DS specific drivers (L2 cache, extra RAM, more CPU cores, frequency scaling)
- NCSD partition type
- add power management to as many drivers as possible (suspend / resume)

- anything else I've forgotten about


# currently implemented devices

- all ARM11 specific devices (SMP, timers, distributed interrupt controller)
- dumb framebuffer output
- HID input (face buttons + LR)
- I2C controller
- SPI controller
- touchscreen and circle pad
- MCU as mfd
- notification LED (only red for now)
- backlight intensity control (only intensity, no on/off yet)
- RTC
- battery and AC charger status report
- reboot and poweroff
- PXI communications with ARM9
- MMC controller (SD and NAND access)
