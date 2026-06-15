/*
 * Copyright (c) 2023 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(auxdisplay_st_stm32_glass_lcd, LOG_LEVEL_DBG);

int main(void)
{
	LOG_INF("Board target: %s", CONFIG_BOARD_TARGET);
	LOG_INF("STM32L476G Discovery 14Seg Glass LCD Sample Application");
	
	/* Retrieve the LCD device structure using the chosen node from Devicetree */
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_auxdisplay));
	
	/* Verify if the device structure is populated and properly instantiated */
	if (dev == NULL) {
		LOG_ERR("Could not find the chosen auxdisplay device node");
		return -ENODEV;
	}

	/* Check if the LCD peripheral driver initialization sequence succeeded at boot */
	if (!device_is_ready(dev)) {
		LOG_ERR("Segment LCD device is not ready for operation");
		return -EBUSY;
	}

	LOG_INF("LCD device ready");

	/* 
	 * Send a test string to the 14-segment glass display using standard Zephyr API.
	 * The driver handles the character decoding matrix in background.
	 */
	const char *test_msg = "ZEPHYR";
	int ret = auxdisplay_write(dev, (const uint8_t *)test_msg, strlen(test_msg));
	if (ret < 0) {
		LOG_ERR("Failed to write to the auxdisplay device (err: %d)", ret);
		return ret;
	}

	LOG_INF("String successfully written to the display!");
	k_msleep(500);

    auxdisplay_clear(dev);
    k_msleep(500);

	const char *msg_all = "\xFF;\xFF;\xFF;\xFF;\xFF;\xFF\xF";
	const char *msg_special_chars = "dm\xB5n  ";
	const char *msg_alpha_1 = "ABCDEF";
	const char *msg_alpha_2 = "GHIJKL";
	const char *msg_alpha_3 = "MNOPQR";
	const char *msg_alpha_4 = "TUVWXY";
	const char *msg_alpha_5 = "Zacioz";
	const char *msg_operators = "+-*\xB0/%%";
	const char *msg_dot_bar        = " . . . . . \x1";
	const char *msg_double_dot_bar = " : : : : : \x2";
	const char *msg_triple_dot_bar = " ; ; ; ; ; \x3";

    while (1) {
        auxdisplay_write(dev, (uint8_t *)msg_all, strlen(msg_all));
        k_msleep(1000);

		auxdisplay_write(dev, (uint8_t *)msg_special_chars, strlen(msg_special_chars));
        k_msleep(1000);
        
		auxdisplay_write(dev, (uint8_t *)msg_alpha_1, strlen(msg_alpha_1));
        k_msleep(1000);
		auxdisplay_write(dev, (uint8_t *)msg_alpha_2, strlen(msg_alpha_2));
        k_msleep(1000);
		auxdisplay_write(dev, (uint8_t *)msg_alpha_3, strlen(msg_alpha_3));
        k_msleep(1000);
		auxdisplay_write(dev, (uint8_t *)msg_alpha_4, strlen(msg_alpha_4));
        k_msleep(1000);
		auxdisplay_write(dev, (uint8_t *)msg_alpha_5, strlen(msg_alpha_5));
        k_msleep(1000);
		auxdisplay_write(dev, (uint8_t *)msg_operators, strlen(msg_operators));
        k_msleep(1000);

		for (int i = 0; i < 3; i++) {
			auxdisplay_write(dev, (uint8_t *)msg_dot_bar, strlen(msg_dot_bar));
			k_msleep(500);

			auxdisplay_write(dev, (uint8_t *)msg_double_dot_bar, strlen(msg_double_dot_bar));
			k_msleep(500);

			auxdisplay_write(dev, (uint8_t *)msg_triple_dot_bar, strlen(msg_triple_dot_bar));
			k_msleep(500);
		}
		k_msleep(500);

		/* Bars: from 0 to 15 */
		uint8_t msg_bar[6*2+1];
		msg_bar[0] = 'B';
		msg_bar[1] = 'A';
		msg_bar[2] = 'R';
		msg_bar[3] = 'S';
		msg_bar[7] = 0;
		for (int i = 0; i < 16; i++) {
			msg_bar[4] = i > 9 ? '1' : ' ';
			msg_bar[5] = i > 9 ? '0' + i - 10 : '0' + i;
			msg_bar[6] = i;
			auxdisplay_write(dev, (uint8_t *)msg_bar, strlen(msg_bar));
			k_msleep(500);
		}
		k_msleep(500);

		/* Bars: from 0% to 100%, step 25% */
		msg_bar[7] = 0;
		for (int i = 0; i < 5; i++) {
			sprintf(msg_bar, "%3d\xB0/%%", i*25);
			msg_bar[6] = (1 << i) - 1;
			auxdisplay_write(dev, (uint8_t *)msg_bar, strlen(msg_bar));
			k_msleep(500);
		}
	}

	return 0;
}
