/* Avoid a failing test due to libpci header symbol shadowing breakage */
#define index shadow_workaround_index
#if !defined __NetBSD__
#include <pci/pci.h>
#else
#include <pciutils/pci.h>
#endif
struct pci_access *pacc;
struct pci_dev *dev = {0};
int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	pacc = pci_alloc();
	dev = pci_get_dev(pacc, dev->bus, dev->dev, 1);
	return 0;
}
