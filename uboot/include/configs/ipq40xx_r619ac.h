/*
 * SPDX-License-Identifier:GPL-2.0-or-later
*/

#ifndef _IPQ40XX_R619AC_H
#define _IPQ40XX_R619AC_H
#define IPQ40XX_R619AC 1

#define MTDIDS              "nand0=nand0"
#define MTDPARTS            "mtdparts=nand0:-(rootfs)"
#define BOOT_MTDPARTS       "mtdparts=nand0:0x4000000@0(rootfs)"

#include <configs/ipq40xx_cdp.h>
#include <ipq40xx_api.h>

#endif /* _IPQ40XX_R619AC_H */
