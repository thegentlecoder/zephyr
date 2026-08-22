/*
 * Copyright 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCK_STM32L4XX_HAL_H
#define MOCK_STM32L4XX_HAL_H

/** This header is included by some other Zephyr sources, so stdint.h must be included here, not
 * only in the C file. And everything must be hidden to the assembly compiler.
 */
#ifndef __ASSEMBLER__

#include "mock_common.h"

typedef struct {
	int n;
} LCD_TypeDef;

typedef struct {
	uint32_t Prescaler;     /*!< Configures the LCD Prescaler.
				     This parameter can be one value of @ref LCD_Prescaler */
	uint32_t Divider;       /*!< Configures the LCD Divider.
				     This parameter can be one value of @ref LCD_Divider */
	uint32_t Duty;          /*!< Configures the LCD Duty.
				     This parameter can be one value of @ref LCD_Duty */
	uint32_t Bias;          /*!< Configures the LCD Bias.
				     This parameter can be one value of @ref LCD_Bias */
	uint32_t VoltageSource; /*!< Selects the LCD Voltage source.
				     This parameter can be one value of @ref LCD_Voltage_Source */
	uint32_t Contrast;      /*!< Configures the LCD Contrast.
				     This parameter can be one value of @ref LCD_Contrast */
	uint32_t DeadTime;      /*!< Configures the LCD Dead Time.
				     This parameter can be one value of @ref LCD_DeadTime */
	uint32_t
		PulseOnDuration; /*!< Configures the LCD Pulse On Duration.
				      This parameter can be one value of @ref LCD_PulseOnDuration */
	uint32_t HighDrive;      /*!< Configures the LCD High Drive.
				       This parameter can be one value of @ref LCD_HighDrive */
	uint32_t BlinkMode;      /*!< Configures the LCD Blink Mode.
				      This parameter can be one value of @ref LCD_BlinkMode */
	uint32_t BlinkFrequency; /*!< Configures the LCD Blink frequency.
				      This parameter can be one value of @ref LCD_BlinkFrequency */
	uint32_t MuxSegment;     /*!< Enable or disable mux segment.
				      This parameter can be one value of @ref LCD_MuxSegment */
} LCD_InitTypeDef;

typedef struct {
	LCD_TypeDef *Instance; /* LCD registers base address */

	LCD_InitTypeDef Init; /* LCD communication parameters */

} LCD_HandleTypeDef;

typedef enum {
	HAL_OK = 0x00,
	HAL_ERROR = 0x01,
	HAL_BUSY = 0x02,
	HAL_TIMEOUT = 0x03
} HAL_StatusTypeDef;

#define __HAL_LCD_ENABLE(__HANDLE__)  UNUSED(__HANDLE__)
#define __HAL_LCD_DISABLE(__HANDLE__) UNUSED(__HANDLE__)

HAL_StatusTypeDef HAL_LCD_Clear(LCD_HandleTypeDef *hlcd);
HAL_StatusTypeDef HAL_LCD_Write(LCD_HandleTypeDef *hlcd, uint32_t RAMRegisterIndex,
				uint32_t RAMRegisterMask, uint32_t Data);
HAL_StatusTypeDef HAL_LCD_UpdateDisplayRequest(LCD_HandleTypeDef *hlcd);
HAL_StatusTypeDef HAL_LCD_Init(LCD_HandleTypeDef *hlcd);

#endif /* __ASSEMBLER__ */

#endif /* MOCK_STM32L4XX_HAL_H */
