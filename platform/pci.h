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

#if defined (__NetBSD__)
#include <pciutils/pci.h>
#else
#include <pci/pci.h>
#endif

#undef index

#endif /* __PLATFORM_PCI_H__ */