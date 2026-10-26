/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2026 Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>
 */

#include "lifecycle.h"

#if CONFIG_BUSPIRATE_SPI == 1
void buspirate_spi_init_fails_invalid_param_test_success(void **state)
{
	/* All of these are rejected before the serial port is opened. */
	const char *const params[] = {
		"dev=/dev/null,pullups=invalid",
		"dev=/dev/null,hiz=invalid",
		"dev=/dev/null,pullups=on,hiz=off",
		"dev=/dev/null,psus=invalid",
	};

	for (size_t i = 0; i < ARRAY_SIZE(params); i++)
		run_init_error_path(state, NULL, &programmer_buspirate_spi, params[i], 1);
}
#else
	SKIP_TEST(buspirate_spi_init_fails_invalid_param_test_success)
#endif /* CONFIG_BUSPIRATE_SPI */
