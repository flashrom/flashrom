/*
 * This is a wrapper for libpci.
 * ...
 */


#ifndef __PLATFORM_PCI_H__
#define __PLATFORM_PCI_H__

/*
 * An old libpci version seems to use the variable name "index" which triggers
 * shadowing warnings on systems which have the index() function in a default
 * #include or as builtin.
 */
#define index shadow_workaround_index

/* Some NetBSDs are using an other include path for pci.h
 * e.g. NetBSD 9.0 on sparc64 pciutils-3.7.0nb2.
 * Other NetBSD platforms and versions uses the default path under pci/pci.h
 */
#if __has_include(<pciutils/pci.h>)
#include <pciutils/pci.h>
#else
#include <pci/pci.h>
#endif

#undef index

#endif /* __PLATFORM_PCI_H__ */
