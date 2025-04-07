### File list
* /usr/src/sys/arch/arm/cortex/pl310.c
* /usr/src/sys/arch/arm/ti/files.ti
* /usr/src/sys/arch/arm/ti/omap3_platform.c
* /usr/src/sys/arch/arm/ti/omap4_prcm.c
* /usr/src/sys/arch/arm/ti/omap_smc.S
* /usr/src/sys/arch/arm/ti/smc.h
* /usr/src/sys/arch/arm/ti/ti_com.c
* /usr/src/sys/arch/arm/ti/ti_div_clock.c
* /usr/src/sys/arch/arm/ti/ti_dpll_clock.c
* /usr/src/sys/arch/arm/ti/ti_gate_clock.c
* /usr/src/sys/arch/arm/ti/ti_omapwugen.c
* /usr/src/sys/arch/arm/ti/ti_prcm.c
* /usr/src/sys/arch/evbarm/conf/GENERIC
* /usr/src/sys/dev/fdt/fixedfactorclock.c

### Boot log
https://dmesgd.nycbug.org/dmesgd?do=view&id=8298

### Build kernel
./build.sh -U -u -O /usr/builds/obj.earmv7hf -T /usr/builds/tool.earmv7hf -j2 -m evbarm -a earmv7hf kernel=GENERIC

### Write image
dd if=armv7.img of=/dev/rld0d bs=1m conv=sync

### U-Boot
cd /usr/pkgsrc/sysutils/u-boot-pandaboard ; make install

mount /dev/rld0e /mnt

cd /usr/pkg/share/u-boot/pandaboard ; cp MLO u-boot.img /mnt

