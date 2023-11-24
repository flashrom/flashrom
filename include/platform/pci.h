/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2022 secunet Security Networks AG
 * (written by Thomas Heijligen <thomas.heijligen@secunet.com)
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
