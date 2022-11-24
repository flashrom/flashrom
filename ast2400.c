/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2016 - 2017 Raptor Engineering, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"

#define PCI_VENDOR_ID_ASPEED		0x1a03

#define ASPEED_MEMMAP_SIZE		(128 * 1024)
#define ASPEED_P2A_OFFSET		0x10000

#define AST2400_SCU_APB_ADDR		0x1e6e2000
#define AST2400_SCU_APB_BRIDGE_OFFSET	(AST2400_SCU_APB_ADDR & 0xffff)
#define AST2400_SCU_PROT_KEY		0x00
#define AST2400_SCU_MISC_CTL		0x2c
#define AST2400_SCU_HW_STRAP		0x70

#define AST2400_SCU_PASSWORD		0x1688a8a8
#define AST2400_SCU_BOOT_SRC_MASK	0x3
#define AST2400_SCU_BOOT_SPI		0x2
#define AST2400_SCU_BOOT_NONE		0x3

#define AST2400_SMC_APB_ADDR		0x1e620000
#define AST2400_SMC_FMC00		0x00
#define AST2400_SMC_CE_CTL(N)		(0x10 + (N * 4))
#define AST2400_SMC_CE_SEG(N)		(0x30 + (N * 4))

#define AST2400_SMC_FLASH_MMIO_ADDR	0x20000000

#define AST2400_SPI_APB_ADDR		0x1e630000
#define AST2400_SPI_CFG			0x00
#define AST2400_SPI_CTL			0x04

#define AST2400_SPI_CFG_WRITE_EN	0x1
#define AST2400_SPI_CMD_FAST_R_MODE	0x1
#define AST2400_SPI_CMD_USER_MODE	0x3
#define AST2400_SPI_CMD_MASK		0x3
#define AST2400_SPI_STOP_CE_ACTIVE	(0x1 << 2)
#define AST2400_SPI_CPOL_1		(0x1 << 4)
#define AST2400_SPI_LSB_FIRST_CTRL	(0x1 << 5)
#define AST2400_SPI_SPEED_MASK		(0xf << 8)
#define AST2400_SPI_IO_MODE_MASK	(0x3 << 28)

#define AST2400_SPI_FLASH_MMIO_ADDR	0x30000000

#define AST2400_WDT_APB_ADDR		0x1e785000
#define AST2400_WDT_APB_BRIDGE_OFFSET	(AST2400_WDT_APB_ADDR & 0xffff)

#define AST2400_WDT1_CTL		0x0c

#define AST2400_WDT_RESET_MODE_MASK	(0x3 << 5)
#define AST2400_WDT_RESET_CPU_ONLY	(0x2 << 5)

uint8_t *ast2400_device_bar = 0;
uint8_t ast2400_device_spi_bus = 0;
uint8_t ast2400_device_halt_cpu = 0;
uint8_t ast2400_device_resume_cpu = 0;
uint8_t ast2400_device_tickle_fw = 0;
uint32_t ast2400_device_flash_mmio_offset = 0;
uint32_t ast2400_device_host_mode = 0;
uint32_t ast2400_original_wdt_conf = 0;

const struct dev_entry bmc_aspeed_ast2400[] = {
	{PCI_VENDOR_ID_ASPEED, 0x2000, OK, "ASPEED", "AST2400" },

	{0},
};

static int ast2400_spi_send_command(struct flashctx *flash,
				   unsigned int writecnt, unsigned int readcnt,
				   const unsigned char *writearr,
				   unsigned char *readarr);

static const struct spi_master spi_master_ast2400 = {
	.max_data_read	= 256,
	.max_data_write	= 256,
	.command	= ast2400_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

static int ast2400_set_a2b_bridge_scu(void)
{
	pci_mmio_writel(0x0, ast2400_device_bar + 0xf000);
	pci_mmio_writel(AST2400_SCU_APB_ADDR & 0xffff0000, ast2400_device_bar + 0xf004);
	pci_mmio_writel(0x1, ast2400_device_bar + 0xf000);

	return 0;
}

static int ast2400_set_a2b_bridge_wdt(void)
{
	pci_mmio_writel(0x0, ast2400_device_bar + 0xf000);
	pci_mmio_writel(AST2400_WDT_APB_ADDR & 0xffff0000, ast2400_device_bar + 0xf004);
	pci_mmio_writel(0x1, ast2400_device_bar + 0xf000);

	return 0;
}

static int ast2400_set_a2b_bridge_smc(void)
{
	pci_mmio_writel(0x0, ast2400_device_bar + 0xf000);
	pci_mmio_writel(AST2400_SMC_APB_ADDR, ast2400_device_bar + 0xf004);
	pci_mmio_writel(0x1, ast2400_device_bar + 0xf000);

	return 0;
}

static int ast2400_set_a2b_bridge_spi(void)
{
	pci_mmio_writel(0x0, ast2400_device_bar + 0xf000);
	pci_mmio_writel(AST2400_SPI_APB_ADDR, ast2400_device_bar + 0xf004);
	pci_mmio_writel(0x1, ast2400_device_bar + 0xf000);

	return 0;
}

static int ast2400_set_a2b_bridge_smc_flash(void)
{
	pci_mmio_writel(0x0, ast2400_device_bar + 0xf000);
	pci_mmio_writel(AST2400_SMC_FLASH_MMIO_ADDR + ast2400_device_flash_mmio_offset, ast2400_device_bar + 0xf004);
	pci_mmio_writel(0x1, ast2400_device_bar + 0xf000);

	return 0;
}

static int ast2400_set_a2b_bridge_spi_flash(void)
{
	pci_mmio_writel(0x0, ast2400_device_bar + 0xf000);
	pci_mmio_writel(AST2400_SPI_FLASH_MMIO_ADDR, ast2400_device_bar + 0xf004);
	pci_mmio_writel(0x1, ast2400_device_bar + 0xf000);

	return 0;
}

static int ast2400_disable_cpu(void) {
	uint32_t dword;

	if (ast2400_device_halt_cpu) {
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SCU_APB_BRIDGE_OFFSET + AST2400_SCU_HW_STRAP);
		if (((dword & AST2400_SCU_BOOT_SRC_MASK) != AST2400_SCU_BOOT_SPI)
			&& ((dword & AST2400_SCU_BOOT_SRC_MASK) != AST2400_SCU_BOOT_NONE)) {	/* NONE permitted to allow for BMC recovery after Ctrl+C or crash */
			msg_perr("CPU halt requested but CPU firmware source is not SPI.\n");
			pci_mmio_writel(0x0, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SCU_APB_BRIDGE_OFFSET + AST2400_SCU_PROT_KEY);
			ast2400_device_halt_cpu = 0;
			return 1;
		}

		/* Disable WDT from issuing full SoC reset
		 * Without this, OpenPOWER systems will crash when the GPIO blocks are reset on WDT timeout
		 */
		msg_pinfo("Configuring P2A bridge for WDT access\n");
		ast2400_set_a2b_bridge_wdt();
		ast2400_original_wdt_conf = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_WDT_APB_BRIDGE_OFFSET + AST2400_WDT1_CTL);
		pci_mmio_writel((ast2400_original_wdt_conf & ~AST2400_WDT_RESET_MODE_MASK) | AST2400_WDT_RESET_CPU_ONLY, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_WDT_APB_BRIDGE_OFFSET + AST2400_WDT1_CTL);

		/* Disable CPU */
		ast2400_set_a2b_bridge_scu();
		pci_mmio_writel((dword & ~AST2400_SCU_BOOT_SRC_MASK) | AST2400_SCU_BOOT_NONE, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SCU_APB_BRIDGE_OFFSET + AST2400_SCU_HW_STRAP);
	}

	return 0;
}

static int ast2400_enable_cpu(void) {
	uint32_t dword;

	if (ast2400_device_halt_cpu && ast2400_device_resume_cpu) {
		/* Re-enable CPU */
		ast2400_set_a2b_bridge_scu();
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SCU_APB_BRIDGE_OFFSET + AST2400_SCU_HW_STRAP);
		pci_mmio_writel((dword & ~AST2400_SCU_BOOT_SRC_MASK) | AST2400_SCU_BOOT_SPI, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SCU_APB_BRIDGE_OFFSET + AST2400_SCU_HW_STRAP);

		/* Reset WDT configuration */
		ast2400_set_a2b_bridge_wdt();
		pci_mmio_writel((ast2400_original_wdt_conf & ~AST2400_WDT_RESET_MODE_MASK) | AST2400_WDT_RESET_CPU_ONLY, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_WDT_APB_BRIDGE_OFFSET + AST2400_WDT1_CTL);
	}

	return 0;
}

static int ast2400_shutdown(void *data) {
	/* Reactivate CPU if previously deactivated */
	ast2400_enable_cpu();

	/* Disable backdoor APB access */
	pci_mmio_writel(0x0, ast2400_device_bar + 0xf000);

	return 0;
}

int ast2400_init(void)
{
	struct pci_dev *dev = NULL;
	uint32_t dword;
	uint8_t divisor;

	char *arg;

	ast2400_device_spi_bus = 0;
	arg = extract_programmer_param("spibus");
	if (arg) {
		if (!strcmp(arg,"host"))
			ast2400_device_host_mode = 1;
		else
			ast2400_device_spi_bus = strtol(arg, NULL, 0);
	}
	free(arg);

	ast2400_device_halt_cpu = 0;
	arg = extract_programmer_param("cpu");
	if (arg && !strcmp(arg,"pause")) {
		ast2400_device_halt_cpu = 1;
		ast2400_device_resume_cpu = 1;
	}
	if (arg && !strcmp(arg,"halt")) {
		ast2400_device_halt_cpu = 1;
		ast2400_device_resume_cpu = 0;
	}
	arg = extract_programmer_param("tickle");
	if (arg && !strcmp(arg,"true"))
		ast2400_device_tickle_fw = 1;
	free(arg);

	if ((ast2400_device_host_mode == 0) && (ast2400_device_spi_bus > 4)) {
		msg_perr("SPI bus number out of range!  Valid values are 0 - 4.\n");
		return 1;
	}

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(bmc_aspeed_ast2400, PCI_BASE_ADDRESS_1);
	if (!dev)
		return 1;

	uintptr_t io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_1);
	if (!io_base_addr)
		return 1;

	msg_pinfo("Detected ASPEED MMIO base address: %p.\n", (void*)io_base_addr);

	ast2400_device_bar = rphysmap("ASPEED", io_base_addr, ASPEED_MEMMAP_SIZE);
	if (ast2400_device_bar == ERROR_PTR)
		return 1;

        if (register_shutdown(ast2400_shutdown, dev))
                return 1;

	io_base_addr += ASPEED_P2A_OFFSET;
	msg_pinfo("ASPEED P2A base address: %p.\n", (void*)io_base_addr);

	msg_pinfo("Configuring P2A bridge for SCU access\n");
	ast2400_set_a2b_bridge_scu();
	pci_mmio_writel(AST2400_SCU_PASSWORD, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SCU_APB_BRIDGE_OFFSET + AST2400_SCU_PROT_KEY);

	dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SCU_APB_BRIDGE_OFFSET + AST2400_SCU_MISC_CTL);
	pci_mmio_writel(dword & ~((0x1 << 24) | (0x2 << 22)), ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SCU_APB_BRIDGE_OFFSET + AST2400_SCU_MISC_CTL);

	/* Halt CPU if requested */
	if (ast2400_disable_cpu())
		return 1;

	msg_pinfo("Configuring P2A bridge for SMC access\n");
	ast2400_set_a2b_bridge_smc();

	if (ast2400_device_host_mode) {
		msg_pinfo("Configuring P2A bridge for SPI access\n");
		ast2400_set_a2b_bridge_spi();

		divisor = 0;	/* Slowest speed for now */

		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SPI_CTL);
		dword &= ~AST2400_SPI_SPEED_MASK;
		dword |= (divisor << 8);
		dword &= ~AST2400_SPI_CPOL_1;
		dword &= ~AST2400_SPI_LSB_FIRST_CTRL;	/* MSB first */
		dword &= ~AST2400_SPI_IO_MODE_MASK;	/* Single bit I/O mode */
		pci_mmio_writel(dword, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SPI_CTL);
	}
	else {
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_FMC00);
		if (((dword >> (ast2400_device_spi_bus * 2)) & 0x3) != 0x2) {
			msg_perr("CE%01x Flash type is not SPI!\n", ast2400_device_spi_bus);
			return 1;
		}

		msg_pinfo("Enabling CE%01x write\n", ast2400_device_spi_bus);
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_FMC00);
		pci_mmio_writel(dword | (0x1 << (16 + ast2400_device_spi_bus)), ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_FMC00);

		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_CE_SEG(ast2400_device_spi_bus));
		ast2400_device_flash_mmio_offset = ((dword >> 16) & 0x3f) * 0x800000;
		msg_pinfo("Using CE%01x offset 0x%08x\n", ast2400_device_spi_bus, ast2400_device_flash_mmio_offset);
	}

	register_spi_master(&spi_master_ast2400);

	return 0;
}

static void ast2400_spi_xfer_data(struct flashctx *flash,
				   unsigned int writecnt, unsigned int readcnt,
				   const unsigned char *writearr,
				   unsigned char *readarr)
{
	unsigned int i;
	uint32_t dword;

	for (i = 0; i < writecnt; i++)
		msg_pspew("[%02x]", writearr[i]);
	msg_pspew("\n");

	for (i = 0; i < writecnt; i=i+4) {
		if ((writecnt - i) < 4)
			break;
		dword = writearr[i];
		dword |= writearr[i + 1] << 8;
		dword |= writearr[i + 2] << 16;
		dword |= writearr[i + 3] << 24;
		pci_mmio_writel(dword, ast2400_device_bar + ASPEED_P2A_OFFSET);
	}
	for (; i < writecnt; i++)
		pci_mmio_writeb(writearr[i], ast2400_device_bar + ASPEED_P2A_OFFSET);
	programmer_delay(1);
	for (i = 0; i < readcnt;) {
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET);
		if (i < readcnt)
			readarr[i] = dword & 0xff;
		i++;
		if (i < readcnt)
			readarr[i] = (dword >> 8) & 0xff;
		i++;
		if (i < readcnt)
			readarr[i] = (dword >> 16) & 0xff;
		i++;
		if (i < readcnt)
			readarr[i] = (dword >> 24) & 0xff;
		i++;
	}

	for (i = 0; i < readcnt; i++)
		msg_pspew("[%02x]", readarr[i]);
	msg_pspew("\n");
}

/* Returns 0 upon success, a negative number upon errors. */
static int ast2400_spi_send_command(struct flashctx *flash,
				   unsigned int writecnt, unsigned int readcnt,
				   const unsigned char *writearr,
				   unsigned char *readarr)
{
	uint32_t dword;

	msg_pspew("%s, cmd=0x%02x, writecnt=%d, readcnt=%d\n", __func__, *writearr, writecnt, readcnt);

	if (ast2400_device_host_mode) {
		/* Set up user command mode */
		ast2400_set_a2b_bridge_spi();
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SPI_CFG);
		pci_mmio_writel(dword | AST2400_SPI_CFG_WRITE_EN, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SPI_CFG);
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SPI_CTL);
		pci_mmio_writel(dword | AST2400_SPI_CMD_USER_MODE, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SPI_CTL);

	        /* Transfer data */
		ast2400_set_a2b_bridge_spi_flash();
		ast2400_spi_xfer_data(flash, writecnt, readcnt, writearr, readarr);

		/* Tear down user command mode */
		ast2400_set_a2b_bridge_spi();
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SPI_CTL);
		pci_mmio_writel((dword & ~AST2400_SPI_CMD_MASK) | AST2400_SPI_CMD_FAST_R_MODE, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SPI_CTL);
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SPI_CFG);
		pci_mmio_writel(dword & ~AST2400_SPI_CFG_WRITE_EN, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SPI_CFG);
	}
	else {
		/* Set up user command mode */
		ast2400_set_a2b_bridge_smc();
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_CE_CTL(ast2400_device_spi_bus));
		pci_mmio_writel(dword | AST2400_SPI_CMD_USER_MODE, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_CE_CTL(ast2400_device_spi_bus));
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_CE_CTL(ast2400_device_spi_bus));
		pci_mmio_writel(dword & ~AST2400_SPI_STOP_CE_ACTIVE, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_CE_CTL(ast2400_device_spi_bus));

		/* Transfer data */
		ast2400_set_a2b_bridge_smc_flash();
		ast2400_spi_xfer_data(flash, writecnt, readcnt, writearr, readarr);

		/* Tear down user command mode */
		ast2400_set_a2b_bridge_smc();
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_CE_CTL(ast2400_device_spi_bus));
		pci_mmio_writel(dword | AST2400_SPI_STOP_CE_ACTIVE, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_CE_CTL(ast2400_device_spi_bus));
		dword = pci_mmio_readl(ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_CE_CTL(ast2400_device_spi_bus));
		pci_mmio_writel((dword & ~AST2400_SPI_CMD_MASK) | AST2400_SPI_CMD_FAST_R_MODE, ast2400_device_bar + ASPEED_P2A_OFFSET + AST2400_SMC_CE_CTL(ast2400_device_spi_bus));
	}

	if (ast2400_device_tickle_fw) {
		ast2400_enable_cpu();
		programmer_delay(100);
		ast2400_disable_cpu();
	}

	return 0;
}
