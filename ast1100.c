/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2017 Raptor Engineering, LLC
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

#define AST1100_SCU_APB_ADDR		0x1e6e2000
#define AST1100_SCU_APB_BRIDGE_OFFSET	(AST1100_SCU_APB_ADDR & 0xffff)
#define AST1100_SCU_PROT_KEY		0x00
#define AST1100_SCU_HW_STRAP		0x70

#define AST1100_SCU_PASSWORD		0x1688a8a8
#define AST1100_SCU_BOOT_SRC_MASK	0x3
#define AST1100_SCU_BOOT_SPI		0x2
#define AST1100_SCU_BOOT_NONE		0x3

#define AST1100_SMC_APB_ADDR		0x16000000
#define AST1100_SMC_SMC00		0x00
#define AST1100_SMC_CE_CTL(N)		(0x4 + (N * 4))

#define AST1100_SMC_SEGMENT_SIZE_MASK	0x3
#define AST1100_SMC_SEGMENT_SIZE_32M	0x0
#define AST1100_SMC_SEGMENT_SIZE_16M	0x1
#define AST1100_SMC_SEGMENT_SIZE_8M	0x2
#define AST1100_SMC_SEGMENT_SIZE_4M	0x3

#define AST1100_SMC_FLASH_MMIO_ADDR	0x10000000

#define AST1100_SPI_CMD_FAST_R_MODE	0x1
#define AST1100_SPI_CMD_USER_MODE	0x3
#define AST1100_SPI_CMD_MASK		0x3
#define AST1100_SPI_STOP_CE_ACTIVE	(0x1 << 2)
#define AST1100_SPI_SPEED_SHIFT		8
#define AST1100_SPI_SPEED_MASK		(0x7 << AST1100_SPI_SPEED_SHIFT)

#define AST1100_SPI_FLASH_MMIO_ADDR	0x30000000

#define AST1100_WDT_APB_ADDR		0x1e785000
#define AST1100_WDT_APB_BRIDGE_OFFSET	(AST1100_WDT_APB_ADDR & 0xffff)

#define AST1100_WDT1_CTR		0x00
#define AST1100_WDT1_CTR_RELOAD		0x04
#define AST1100_WDT1_CTR_RESTART	0x08
#define AST1100_WDT1_CTL		0x0c

#define AST1100_WDT_SET_CLOCK		(0x1 << 4)
#define AST1100_WDT_RESET_SYSTEM	(0x1 << 1)
#define AST1100_WDT_ENABLE		(0x1 << 0)

uint8_t *ast1100_device_bar = 0;
uint8_t ast1100_device_spi_bus = 0;
uint8_t ast1100_device_spi_speed = 0;
uint8_t ast1100_device_halt_cpu = 0;
uint8_t ast1100_device_reset_cpu = 0;
uint8_t ast1100_device_resume_cpu = 0;
uint8_t ast1100_device_tickle_fw = 0;
uint32_t ast1100_device_flash_mmio_offset = 0;
uint32_t ast1100_original_wdt_conf = 0;

const struct dev_entry bmc_aspeed_ast1100[] = {
	{PCI_VENDOR_ID_ASPEED, 0x2000, OK, "ASPEED", "AST1100" },

	{0},
};

static int ast1100_spi_send_command(struct flashctx *flash,
				   unsigned int writecnt, unsigned int readcnt,
				   const unsigned char *writearr,
				   unsigned char *readarr);

static const struct spi_master spi_master_ast1100 = {
	.max_data_read	= 256,
	.max_data_write	= 256,
	.command	= ast1100_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

static int ast1100_set_a2b_bridge_scu(void)
{
	pci_mmio_writel(0x0, ast1100_device_bar + 0xf000);
	pci_mmio_writel(AST1100_SCU_APB_ADDR & 0xffff0000, ast1100_device_bar + 0xf004);
	pci_mmio_writel(0x1, ast1100_device_bar + 0xf000);

	return 0;
}

static int ast1100_set_a2b_bridge_wdt(void)
{
	pci_mmio_writel(0x0, ast1100_device_bar + 0xf000);
	pci_mmio_writel(AST1100_WDT_APB_ADDR & 0xffff0000, ast1100_device_bar + 0xf004);
	pci_mmio_writel(0x1, ast1100_device_bar + 0xf000);

	return 0;
}

static int ast1100_set_a2b_bridge_smc(void)
{
	pci_mmio_writel(0x0, ast1100_device_bar + 0xf000);
	pci_mmio_writel(AST1100_SMC_APB_ADDR, ast1100_device_bar + 0xf004);
	pci_mmio_writel(0x1, ast1100_device_bar + 0xf000);

	return 0;
}

static int ast1100_set_a2b_bridge_smc_flash(void)
{
	pci_mmio_writel(0x0, ast1100_device_bar + 0xf000);
	pci_mmio_writel(AST1100_SMC_FLASH_MMIO_ADDR + ast1100_device_flash_mmio_offset, ast1100_device_bar + 0xf004);
	pci_mmio_writel(0x1, ast1100_device_bar + 0xf000);

	return 0;
}

static int ast1100_disable_cpu(void) {
	uint32_t dword;

	if (ast1100_device_halt_cpu) {
		dword = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SCU_APB_BRIDGE_OFFSET + AST1100_SCU_HW_STRAP);
		if (((dword & AST1100_SCU_BOOT_SRC_MASK) != AST1100_SCU_BOOT_SPI)
			&& ((dword & AST1100_SCU_BOOT_SRC_MASK) != AST1100_SCU_BOOT_NONE)) {	/* NONE permitted to allow for BMC recovery after Ctrl+C or crash */
			msg_perr("CPU halt requested but CPU firmware source is not SPI.\n");
			pci_mmio_writel(0x0, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SCU_APB_BRIDGE_OFFSET + AST1100_SCU_PROT_KEY);
			ast1100_device_halt_cpu = 0;
			return 1;
		}

		/* Disable CPU */
		ast1100_set_a2b_bridge_scu();
		pci_mmio_writel((dword & ~AST1100_SCU_BOOT_SRC_MASK) | AST1100_SCU_BOOT_NONE, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SCU_APB_BRIDGE_OFFSET + AST1100_SCU_HW_STRAP);
		ast1100_original_wdt_conf = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_WDT_APB_BRIDGE_OFFSET + AST1100_WDT1_CTL);
		pci_mmio_writel(ast1100_original_wdt_conf & 0xffff0, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_WDT_APB_BRIDGE_OFFSET + AST1100_WDT1_CTL);
	}

	return 0;
}

static int ast1100_enable_cpu(void) {
	uint32_t dword;

	if (ast1100_device_halt_cpu && ast1100_device_resume_cpu) {
		/* Re-enable CPU */
		ast1100_set_a2b_bridge_scu();
		dword = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SCU_APB_BRIDGE_OFFSET + AST1100_SCU_HW_STRAP);
		pci_mmio_writel((dword & ~AST1100_SCU_BOOT_SRC_MASK) | AST1100_SCU_BOOT_SPI, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SCU_APB_BRIDGE_OFFSET + AST1100_SCU_HW_STRAP);
	}

	return 0;
}

static int ast1100_reset_cpu(void) {
	if (ast1100_device_reset_cpu) {
		/* Disable WDT from issuing full SoC reset
		 * Without this, OpenPOWER systems will crash when the GPIO blocks are reset on WDT timeout
		 */
		msg_pinfo("Configuring P2A bridge for WDT access\n");
		ast1100_set_a2b_bridge_wdt();
		ast1100_original_wdt_conf = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_WDT_APB_BRIDGE_OFFSET + AST1100_WDT1_CTL);

		/* Initiate reset */
		msg_pinfo("Setting WDT to reset CPU immediately\n");
		pci_mmio_writel(ast1100_original_wdt_conf & 0xffff0, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_WDT_APB_BRIDGE_OFFSET + AST1100_WDT1_CTL);
		pci_mmio_writel(0xec08ce00, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_WDT_APB_BRIDGE_OFFSET + AST1100_WDT1_CTR_RELOAD);
		pci_mmio_writel(0x4755, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_WDT_APB_BRIDGE_OFFSET + AST1100_WDT1_CTR_RESTART);
		pci_mmio_writel(AST1100_WDT_SET_CLOCK, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_WDT_APB_BRIDGE_OFFSET + AST1100_WDT1_CTL);
		pci_mmio_writel(AST1100_WDT_RESET_SYSTEM | AST1100_WDT_ENABLE, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_WDT_APB_BRIDGE_OFFSET + AST1100_WDT1_CTL);

	}

	return 0;
}

static int ast1100_shutdown(void *data) {
	/* Reactivate CPU if previously deactivated */
	ast1100_enable_cpu();

	/* Reset CPU if requested */
	ast1100_reset_cpu();

	/* Disable backdoor APB access */
	pci_mmio_writel(0x0, ast1100_device_bar + 0xf000);

	return 0;
}

int ast1100_init(void)
{
	struct pci_dev *dev = NULL;
	uint32_t dword;

	char *arg;

	ast1100_device_spi_bus = 0;
	arg = extract_programmer_param("spibus");
	if (arg)
		ast1100_device_spi_bus = strtol(arg, NULL, 0);
	free(arg);

	ast1100_device_spi_speed = 0;
	arg = extract_programmer_param("spispeed");
	if (arg)
		ast1100_device_spi_speed = strtol(arg, NULL, 0);
	free(arg);

	ast1100_device_halt_cpu = 0;
	arg = extract_programmer_param("cpu");
	if (arg && !strcmp(arg,"pause")) {
		ast1100_device_halt_cpu = 1;
		ast1100_device_resume_cpu = 1;
		ast1100_device_reset_cpu = 0;
	}
	else if (arg && !strcmp(arg,"halt")) {
		ast1100_device_halt_cpu = 1;
		ast1100_device_resume_cpu = 0;
		ast1100_device_reset_cpu = 0;
	}
	else if (arg && !strcmp(arg,"reset")) {
		ast1100_device_halt_cpu = 1;
		ast1100_device_resume_cpu = 1;
		ast1100_device_reset_cpu = 1;
	}
	else if (arg) {
		msg_perr("Invalid CPU option!  Valid values are: pause | halt | reset\n");
		return 1;
	}
	arg = extract_programmer_param("tickle");
	if (arg && !strcmp(arg,"true"))
		ast1100_device_tickle_fw = 1;
	free(arg);

	if (ast1100_device_spi_bus > 2) {
		msg_perr("SPI bus number out of range!  Valid values are 0 - 2.\n");
		return 1;
	}

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(bmc_aspeed_ast1100, PCI_BASE_ADDRESS_1);
	if (!dev)
		return 1;

	uintptr_t io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_1);
	if (!io_base_addr)
		return 1;

	msg_pinfo("Detected ASPEED MMIO base address: %p.\n", (void*)io_base_addr);

	ast1100_device_bar = rphysmap("ASPEED", io_base_addr, ASPEED_MEMMAP_SIZE);
	if (ast1100_device_bar == ERROR_PTR)
		return 1;

        if (register_shutdown(ast1100_shutdown, dev))
                return 1;

	io_base_addr += ASPEED_P2A_OFFSET;
	msg_pinfo("ASPEED P2A base address: %p.\n", (void*)io_base_addr);

	msg_pinfo("Configuring P2A bridge for SCU access\n");
	ast1100_set_a2b_bridge_scu();
	pci_mmio_writel(AST1100_SCU_PASSWORD, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SCU_APB_BRIDGE_OFFSET + AST1100_SCU_PROT_KEY);

	/* Halt CPU if requested */
	if (ast1100_disable_cpu())
		return 1;

	msg_pinfo("Configuring P2A bridge for SMC access\n");
	ast1100_set_a2b_bridge_smc();

	dword = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_SMC00);
	if (((dword >> ((ast1100_device_spi_bus * 2) + 4)) & 0x3) != 0x2) {
		msg_perr("CE%01x Flash type is not SPI!\n", ast1100_device_spi_bus);
		return 1;
	}

	msg_pinfo("Setting CE%01x SPI relative clock speed to %d\n", ast1100_device_spi_bus, ast1100_device_spi_speed);
	dword = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_CE_CTL(ast1100_device_spi_bus));
	dword &= ~AST1100_SPI_SPEED_MASK;
	pci_mmio_writel(dword | ((ast1100_device_spi_speed << AST1100_SPI_SPEED_SHIFT) & AST1100_SPI_SPEED_MASK), ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_CE_CTL(ast1100_device_spi_bus));

	msg_pinfo("Enabling CE%01x write\n", ast1100_device_spi_bus);
	dword = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_SMC00);
	pci_mmio_writel(dword | (0x1 << (10 + ast1100_device_spi_bus)), ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_SMC00);

	dword = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_SMC00);
	dword &= AST1100_SMC_SEGMENT_SIZE_MASK;
	switch (dword & AST1100_SMC_SEGMENT_SIZE_MASK) {
		case AST1100_SMC_SEGMENT_SIZE_32M:
			ast1100_device_flash_mmio_offset = 0x2000000;
			break;
		case AST1100_SMC_SEGMENT_SIZE_16M:
			ast1100_device_flash_mmio_offset = 0x1000000;
			break;
		case AST1100_SMC_SEGMENT_SIZE_8M:
			ast1100_device_flash_mmio_offset = 0x800000;
			break;
		case AST1100_SMC_SEGMENT_SIZE_4M:
			ast1100_device_flash_mmio_offset = 0x400000;
			break;
		default:
			ast1100_device_flash_mmio_offset = 0x2000000;
	}
	msg_pinfo("Segment size: 0x%08x\n", ast1100_device_flash_mmio_offset);

	ast1100_device_flash_mmio_offset = ast1100_device_flash_mmio_offset * ast1100_device_spi_bus;
	msg_pinfo("Using CE%01x offset 0x%08x\n", ast1100_device_spi_bus, ast1100_device_flash_mmio_offset);

	register_spi_master(&spi_master_ast1100);

	return 0;
}

static void ast1100_spi_xfer_data(struct flashctx *flash,
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
		pci_mmio_writel(dword, ast1100_device_bar + ASPEED_P2A_OFFSET);
	}
	for (; i < writecnt; i++)
		pci_mmio_writeb(writearr[i], ast1100_device_bar + ASPEED_P2A_OFFSET);
	programmer_delay(1);
	for (i = 0; i < readcnt;) {
		dword = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET);
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
static int ast1100_spi_send_command(struct flashctx *flash,
				   unsigned int writecnt, unsigned int readcnt,
				   const unsigned char *writearr,
				   unsigned char *readarr)
{
	uint32_t dword;

	msg_pspew("%s, cmd=0x%02x, writecnt=%d, readcnt=%d\n", __func__, *writearr, writecnt, readcnt);

	/* Set up user command mode */
	ast1100_set_a2b_bridge_smc();
	dword = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_CE_CTL(ast1100_device_spi_bus));
	pci_mmio_writel(dword | AST1100_SPI_CMD_USER_MODE, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_CE_CTL(ast1100_device_spi_bus));
	dword = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_CE_CTL(ast1100_device_spi_bus));
	pci_mmio_writel(dword & ~AST1100_SPI_STOP_CE_ACTIVE, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_CE_CTL(ast1100_device_spi_bus));

	/* Transfer data */
	ast1100_set_a2b_bridge_smc_flash();
	ast1100_spi_xfer_data(flash, writecnt, readcnt, writearr, readarr);

	/* Tear down user command mode */
	ast1100_set_a2b_bridge_smc();
	dword = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_CE_CTL(ast1100_device_spi_bus));
	pci_mmio_writel(dword | AST1100_SPI_STOP_CE_ACTIVE, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_CE_CTL(ast1100_device_spi_bus));
	dword = pci_mmio_readl(ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_CE_CTL(ast1100_device_spi_bus));
	pci_mmio_writel((dword & ~AST1100_SPI_CMD_MASK) | AST1100_SPI_CMD_FAST_R_MODE, ast1100_device_bar + ASPEED_P2A_OFFSET + AST1100_SMC_CE_CTL(ast1100_device_spi_bus));

	if (ast1100_device_tickle_fw) {
		ast1100_enable_cpu();
		programmer_delay(100);
		ast1100_disable_cpu();
	}

	return 0;
}
