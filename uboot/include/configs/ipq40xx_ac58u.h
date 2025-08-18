/*
 * SPDX-License-Identifier:GPL-2.0-or-later
*/

#ifndef _IPQ40XX_AC58U_H
#define _IPQ40XX_AC58U_H
#define IPQ40XX_AC58U       1
#define CONFIG_ALT_BANNER   1 /* Print the current model */

#define MTDIDS              "nand1=nand1"
#define MTDPARTS            "mtdparts=nand1:-(UBI_DEV)"
#define BOOT_MTDPARTS       "mtdparts=nand1:0x8000000@0(UBI_DEV)"

#include <configs/ipq40xx_cdp.h>
#include <ipq40xx_api.h>

#endif /* _IPQ40XX_AC58U_H */
