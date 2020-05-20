/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <include/test.h>
#include "tests.h"

#include <stdio.h>

/* redefinitions/wrapping */
void __wrap_physunmap(void *virt_addr, size_t len)
{
	fprintf(stderr, "%s\n", __func__);
}
void *__wrap_physmap(const char *descr, uintptr_t phys_addr, size_t len)
{
	fprintf(stderr, "%s\n", __func__);
	return NULL;
}

int main(void)
{
	int ret = 0;

	return ret;
}
