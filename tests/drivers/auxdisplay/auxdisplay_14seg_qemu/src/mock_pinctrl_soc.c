/*
 * Copyright (c) 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include "mock_common.h"
/** pinctrl_soc.h is included via pinctrl.h, that exists also in the QEMU environment.
 * For this reason, it must be mocked with a file having the same name.
 */
#include "pinctrl_soc.h"

struct pinctrl_dev_config;

int pinctrl_lookup_state(const struct pinctrl_dev_config *config, uint8_t state, uint8_t *idx)
{
	UNUSED(config);
	UNUSED(state);
	if (idx) {
		*idx = 0;
	}
	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	UNUSED(pins);
	UNUSED(pin_cnt);
	UNUSED(reg);
	return 0;
}
