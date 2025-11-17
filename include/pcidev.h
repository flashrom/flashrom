/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2025 Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>
 */

#ifndef __PCIDEV_H__
#define __PCIDEV_H__

#include "platform/pci.h"
#include "programmer.h"

/* e.g. config space only */
#define PCIDEV_NO_BAR_ACCESS_NEEDED	(-1)

// FIXME: This needs to be local, not global(?)
extern struct pci_access *pacc;

int pci_init_common(void);
uintptr_t pcidev_readbar(struct pci_dev *dev, int bar);
struct pci_dev *pcidev_init(const struct programmer_cfg *cfg, const struct dev_entry *devs, int bar);
struct pci_dev *pcidev_scandev(struct pci_filter *filter, struct pci_dev *start);
struct pci_dev *pcidev_getdevfn(struct pci_dev *dev, const int func);
struct pci_dev *pcidev_find_vendorclass(uint16_t vendor, uint16_t devclass);
struct pci_dev *pcidev_card_find(uint16_t vendor, uint16_t device, uint16_t card_vendor, uint16_t card_device);
struct pci_dev *pcidev_find(uint16_t vendor, uint16_t device);

/* rpci_write_* are reversible writes. The original PCI config space register
 * contents will be restored on shutdown.
 * To clone the pci_dev instances internally, the `pacc` global
 * variable has to reference a pci_access method that is compatible
 * with the given pci_dev handle. The referenced pci_access (not
 * the variable) has to stay valid until the shutdown handlers are
 * finished.
 */
int rpci_write_byte(struct pci_dev *dev, int reg, uint8_t data);
int rpci_write_word(struct pci_dev *dev, int reg, uint16_t data);
int rpci_write_long(struct pci_dev *dev, int reg, uint32_t data);

#endif
