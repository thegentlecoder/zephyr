/*
 * Copyright 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCK_STM32_CLOCK_CONTROL_H
#define MOCK_STM32_CLOCK_CONTROL_H

#define STM32_CLOCK_DIV_SHIFT 12

struct stm32_pclken {
	uint32_t bus: STM32_CLOCK_DIV_SHIFT;
	uint32_t div: (32 - STM32_CLOCK_DIV_SHIFT);
	uint32_t enr;
};

#endif /* MOCK_STM32_CLOCK_CONTROL_H */
