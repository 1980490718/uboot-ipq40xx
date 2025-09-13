/*
 * SPDX-License-Identifier:GPL-2.0-or-later
*/

#ifndef _IPQ40XX_E2600ACC2_H
#define _IPQ40XX_E2600ACC2_H

#define MTDIDS              "nand0=nand0"
#define MTDPARTS            "mtdparts=nand0:-(ubi)"
#define BOOT_MTDPARTS       "mtdparts=nand0:0x4000000@0(ubi)"

#include <configs/ipq40xx_cdp.h>
#include <ipq40xx_api.h>

#endif /* _IPQ40XX_E2600ACC2_H */
