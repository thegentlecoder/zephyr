/*
 * Copyright (c) 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include "mock_stm32l4xx_hal.h"

HAL_StatusTypeDef HAL_LCD_Clear(LCD_HandleTypeDef *hlcd)
{
	UNUSED(hlcd);
	/* No operation in QEMU */
	return HAL_OK;
}

HAL_StatusTypeDef HAL_LCD_Write(LCD_HandleTypeDef *hlcd, uint32_t RAMRegisterIndex,
				uint32_t RAMRegisterMask, uint32_t Data)
{
	UNUSED(hlcd);
	UNUSED(RAMRegisterIndex);
	UNUSED(RAMRegisterMask);
	UNUSED(Data);
	/* No operation in QEMU */
	return HAL_OK;
}

HAL_StatusTypeDef HAL_LCD_UpdateDisplayRequest(LCD_HandleTypeDef *hlcd)
{
	UNUSED(hlcd);
	/* No operation in QEMU */
	return HAL_OK;
}

HAL_StatusTypeDef HAL_LCD_Init(LCD_HandleTypeDef *hlcd)
{
	/* No operation in QEMU */
	return HAL_OK;
}
