/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2022 secunet Security Networks AG and Thomas Heijligen <thomas.heijligen@secunet.com>
 */

#ifndef __PLATFORM_PCI_H__
#define __PLATFORM_PCI_H__

/* Some NetBSDs are using an other include path for pci.h
 * e.g. NetBSD 9.0 on sparc64 pciutils-3.7.0nb2.
 * Other NetBSD platforms and versions uses the default path under pci/pci.h
 */
#ifdef HAVE_PCIUTILS_PCI_H
#include <pciutils/pci.h>
#else
#include <pci/pci.h>
#endif /* HAVE_PCIUTILS_PCI_H */

#endif /* __PLATFORM_PCI_H__ */
