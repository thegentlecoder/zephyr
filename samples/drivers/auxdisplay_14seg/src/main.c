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

LOG_MODULE_REGISTER(auxdisplay_sample_stm32l476g_disco, LOG_LEVEL_DBG);

int main(void)
{
	printk("Aux Display 1! %s\n", CONFIG_BOARD_TARGET);
	//int rc;
	
	//const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(auxdisplay_0));
	/* Retrieve the LCD device structure using the chosen node from Devicetree */
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_auxdisplay));

	LOG_INF("STM32L476G Discovery Glass LCD Test Application");
	
	//uint8_t data[64];

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

	LOG_INF("LCD device is ready. Writing string payload...");

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

	// rc = auxdisplay_cursor_set_enabled(dev, true);

	// if (rc != 0) {
	// 	LOG_ERR("Failed to enable cursor: %d", rc);
	// }

	// snprintk(data, sizeof(data), "Hello world from %s", CONFIG_BOARD);
	// rc = auxdisplay_write(dev, data, strlen(data));
	// if (rc != 0) {
	// 	LOG_ERR("Failed to write data: %d", rc);
	// 	return -3;
	// }

    auxdisplay_clear(dev);
    k_msleep(500);

	const char *msg_all = "\xFF;\xFF;\xFF;\xFF;\xFF;\xFF\xF";

	const char *msg1 = "1.2:3;A\xB5\xFF;";
	const char *msg2 = " . . . . . \x1";
	const char *msg3 = " : : : : : \x2";
	const char *msg4 = " ; ; ; ; ; \x3";

    while (1) {
        auxdisplay_write(dev, (uint8_t *)msg_all, strlen(msg_all));
        k_msleep(1000);

		auxdisplay_write(dev, (uint8_t *)msg1, strlen(msg1));
        k_msleep(1000);
        
		for (int i = 0; i < 3; i++) {
			auxdisplay_write(dev, (uint8_t *)msg2, strlen(msg2));
			k_msleep(500);

			auxdisplay_write(dev, (uint8_t *)msg3, strlen(msg3));
			k_msleep(500);

			auxdisplay_write(dev, (uint8_t *)msg4, strlen(msg4));
			k_msleep(500);
		}
		k_msleep(500);

		uint8_t msg_bar[6*2+1];
		msg_bar[0] = 'B';
		msg_bar[1] = 'A';
		msg_bar[2] = 'R';
		msg_bar[3] = ' ';
		msg_bar[7] = 0;
		for (int i = 0; i < 16; i++) {
			msg_bar[4] = i > 9 ? '1' : ' ';
			msg_bar[5] = i > 9 ? '0' + i - 10 : '0' + i;
			msg_bar[6] = i;
			auxdisplay_write(dev, (uint8_t *)msg_bar, strlen(msg_bar));
			k_msleep(500);
		}
		k_msleep(500);

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
