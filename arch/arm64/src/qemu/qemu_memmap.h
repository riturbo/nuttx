/****************************************************************************
 * arch/arm64/src/a64/hardware/a64_memorymap.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM64_SRC_SUNXI_A133_HARDWARE_A133_MEMORYMAP_H
#define __ARCH_ARM64_SRC_SUNXI_A133_HARDWARE_A133_MEMORYMAP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Memory regions */
#define SUNXI_ROM_BASE			0x00000000
#define SUNXI_ROM_SIZE			0x00010000
#define SUNXI_SRAM_BASE			0x00020000
#define SUNXI_SRAM_SIZE			0x000f8000
#define SUNXI_SRAM_A1_BASE		0x00020000
#define SUNXI_SRAM_A1_SIZE		0x00008000
#define SUNXI_SRAM_A2_BASE		0x00100000
#define SUNXI_SRAM_A2_BL31_OFFSET	0x00004000
#define SUNXI_SRAM_A2_SIZE		0x00018000
#define SUNXI_SRAM_C_BASE		0x00028000
#define SUNXI_SRAM_C_SIZE		0x0001e000
#define SUNXI_DEV_BASE			0x01000000
#define SUNXI_DEV_SIZE			0x09000000
#define SUNXI_DRAM_BASE			0x40000000
#define SUNXI_DRAM_VIRT_BASE		0x0a000000

/* Memory-mapped devices */
#define SUNXI_SYSCON_BASE		0x03000000
#define SUNXI_CPUCFG_BASE		0x09010000
#define SUNXI_SID_BASE			0x03006000
#define SUNXI_DMA_BASE			0x03002000
#define SUNXI_MSGBOX_BASE		0x03003000
#define SUNXI_CCU_BASE			0x03001000
#define SUNXI_PIO_BASE			0x0300b000
#define SUNXI_SPC_BASE			0x03008000
#define SUNXI_TIMER_BASE		0x03009000
#define SUNXI_WDOG_BASE			0x030090a0
#define SUNXI_THS_BASE			0x05070400
#define SUNXI_UART0_BASE		0x05000000
#define SUNXI_UART1_BASE		0x05000400
#define SUNXI_UART2_BASE		0x05000800
#define SUNXI_UART3_BASE		0x05000c00
#define SUNXI_I2C0_BASE			0x05002000
#define SUNXI_I2C1_BASE			0x05002400
#define SUNXI_I2C2_BASE			0x05002800
#define SUNXI_I2C3_BASE			0x05002c00
#define SUNXI_SPI0_BASE			0x05010000
#define SUNXI_SPI1_BASE			0x05011000
#define SUNXI_SCU_BASE			0x03020000
#define SUNXI_GICD_BASE			0x03021000
#define SUNXI_GICC_BASE			0x03022000
#define SUNXI_R_TIMER_BASE		0x07020000
#define SUNXI_R_INTC_BASE		0x07021000
#define SUNXI_R_WDOG_BASE		0x07020400
#define SUNXI_R_PRCM_BASE		0x07010000
#define SUNXI_R_TWD_BASE		0x07020800
#define SUNXI_R_CPUCFG_BASE		0x07000400
#define SUNXI_R_I2C_BASE		0x07081400
#define SUNXI_R_RSB_BASE		0x07083000
#define SUNXI_R_UART_BASE		0x07080000
#define SUNXI_R_PIO_BASE		0x07022000

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions Prototypes
 ****************************************************************************/

#endif /* __ARCH_ARM64_SRC_SUNXI_A133_HARDWARE_A133_MEMORYMAP_H */
