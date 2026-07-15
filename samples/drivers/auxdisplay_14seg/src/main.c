/*
 * Copyright (c) 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(auxdisplay_sample, LOG_LEVEL_DBG);

/* To perform some tests, at least six columns are needed */
#define EXPECTED_LCD_COLUMNS_MIN 6
/* The text buffer can contain a digit/letter char in each position and a point char
 * for each position. The point char is optional.
 */
#define FIXED_TEXT_MESSAGE_LEN   EXPECTED_LCD_COLUMNS_MIN * 2
#define FIXED_TEXT_MESSAGE_SIZE  FIXED_TEXT_MESSAGE_LEN + 1

int main(void)
{
	struct auxdisplay_capabilities caps;
	int ret;

	LOG_INF("Board target: %s", CONFIG_BOARD_TARGET);
	LOG_INF("STM32L476G Discovery 14Seg Glass LCD Sample Application");

	/* Retrieve the LCD device structure using the chosen node from Devicetree */
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(auxdisplay_0));

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
	const char *msg_zephyr = "ZEPHYR";

	ret = auxdisplay_write(dev, (const uint8_t *)msg_zephyr, strlen(msg_zephyr));
	if (ret < 0) {
		LOG_ERR("Failed to write to the auxdisplay device (err: %d)", ret);
		return ret;
	}

	LOG_INF("String successfully written to the display!");
	k_msleep(500);

	auxdisplay_clear(dev);
	k_msleep(500);

	ret = auxdisplay_capabilities_get(dev, &caps);
	if (ret < 0) {
		LOG_ERR("Failed to read the auxdisplay capabilities (err: %d)", ret);
	}
	if (caps.columns < EXPECTED_LCD_COLUMNS_MIN) {
		LOG_WRN("This auxdisplay example needs at least 6 columns, found just %d, texts "
			"are going to be truncated",
			caps.columns);
		/* Don't return here, the driver must be tolerant even receiving texts longer than
		 * the column count.
		 */
	}

	uint8_t custom_character_full_data[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	struct auxdisplay_character custom_character_full = {
		.index = 0,
		.data = custom_character_full_data,
		.character_code = 0,
	};
	ret = auxdisplay_custom_character_set(dev, &custom_character_full);
	if (ret < 0) {
		LOG_ERR("Failed to set the full custom character (err: %d)", ret);
	}
	uint8_t custom_character_degree_data[] = {0xff, 0xff, 0x0, 0x0,  0x0, 0xff, 0xff,
						  0x0,  0x0,  0x0, 0xff, 0x0, 0x0,  0x0};
	struct auxdisplay_character custom_character_degree = {
		.index = 0,
		.data = custom_character_degree_data,
		.character_code = 0,
	};
	ret = auxdisplay_custom_character_set(dev, &custom_character_degree);
	if (ret < 0) {
		LOG_ERR("Failed to set the degree custom character (err: %d)", ret);
	}
	uint8_t custom_character_low_ring_data[] = {0x0, 0x0, 0xff, 0xff, 0xff, 0x0, 0xff,
						    0x0, 0x0, 0x0,  0xff, 0x0,  0x0, 0x0};
	struct auxdisplay_character custom_character_low_ring = {
		.index = 0,
		.data = custom_character_low_ring_data,
		.character_code = 0,
	};
	ret = auxdisplay_custom_character_set(dev, &custom_character_low_ring);
	if (ret < 0) {
		LOG_ERR("Failed to set the low_ring custom character (err: %d)", ret);
	}
	uint8_t custom_character_prefix_d_data[] = {0x0, 0xff, 0xff, 0xff, 0xff, 0x0, 0xff,
						    0x0, 0x0,  0x0,  0xff, 0x0,  0x0, 0x0};
	struct auxdisplay_character custom_character_prefix_d = {
		.index = 0,
		.data = custom_character_prefix_d_data,
		.character_code = 0,
	};
	ret = auxdisplay_custom_character_set(dev, &custom_character_prefix_d);
	if (ret < 0) {
		LOG_ERR("Failed to set the prefix_d custom character (err: %d)", ret);
	}
	uint8_t custom_character_prefix_c_data[] = {0x0, 0x0, 0x0, 0xff, 0xff, 0x0, 0xff,
						    0x0, 0x0, 0x0, 0xff, 0x0,  0x0, 0x0};
	struct auxdisplay_character custom_character_prefix_c = {
		.index = 0,
		.data = custom_character_prefix_c_data,
		.character_code = 0,
	};
	ret = auxdisplay_custom_character_set(dev, &custom_character_prefix_c);
	if (ret < 0) {
		LOG_ERR("Failed to set the prefix_c custom character (err: %d)", ret);
	}
	uint8_t custom_character_prefix_m_data[] = {0x0, 0x0, 0xff, 0x0,  0xff, 0x0,  0xff,
						    0x0, 0x0, 0x0,  0xff, 0x0,  0xff, 0x0};
	struct auxdisplay_character custom_character_prefix_m = {
		.index = 0,
		.data = custom_character_prefix_m_data,
		.character_code = 0,
	};
	ret = auxdisplay_custom_character_set(dev, &custom_character_prefix_m);
	if (ret < 0) {
		LOG_ERR("Failed to set the prefix_m custom character (err: %d)", ret);
	}
	uint8_t custom_character_prefix_u_data[] = {0x0, 0x0, 0xff, 0xff, 0xff, 0x0, 0x0,
						    0x0, 0x0, 0x0,  0x0,  0x0,  0x0, 0x0};
	struct auxdisplay_character custom_character_prefix_u = {
		.index = 0,
		.data = custom_character_prefix_u_data,
		.character_code = 0,
	};
	ret = auxdisplay_custom_character_set(dev, &custom_character_prefix_u);
	if (ret < 0) {
		LOG_ERR("Failed to set the prefix_u custom character (err: %d)", ret);
	}
	uint8_t custom_character_prefix_n_data[] = {0x0, 0x0, 0xff, 0x0,  0x0, 0x0,  0x0,
						    0x0, 0x0, 0x0,  0xff, 0x0, 0xff, 0x0};
	struct auxdisplay_character custom_character_prefix_n = {
		.index = 0,
		.data = custom_character_prefix_n_data,
		.character_code = 0,
	};
	ret = auxdisplay_custom_character_set(dev, &custom_character_prefix_n);
	if (ret < 0) {
		LOG_ERR("Failed to set the prefix_n custom character (err: %d)", ret);
	}

	const char *msg_alpha_num_1 = "ABCDEF";
	const char *msg_alpha_num_2 = "GHIJKL";
	const char *msg_alpha_num_3 = "MNOPQR";
	const char *msg_alpha_num_4 = "STUVWX";
	const char *msg_alpha_num_5 = "YZ0123";
	const char *msg_alpha_num_6 = "456789";
	const char msg_unit_prefixes[] = {/* d c m u n */
					  custom_character_prefix_d.character_code,
					  custom_character_prefix_c.character_code,
					  custom_character_prefix_m.character_code,
					  custom_character_prefix_u.character_code,
					  custom_character_prefix_n.character_code,
					  ' ',
					  0};
	const char *msg_operators = "+-*/  ";
	const char msg_symbols[] = {'_', '(',
				    ')', custom_character_degree.character_code,
				    '/', custom_character_low_ring.character_code,
				    0};

	const char *msg_dot = " . . . . . .";
	const char *msg_double_dot = " : : : : : :";

	uint8_t msg_bar[FIXED_TEXT_MESSAGE_SIZE];

	while (1) {
		msg_bar[0] = custom_character_full.character_code;
		msg_bar[1] = custom_character_full.character_code;
		msg_bar[2] = custom_character_full.character_code;
		msg_bar[3] = custom_character_full.character_code;
		msg_bar[4] = custom_character_full.character_code;
		msg_bar[5] = custom_character_full.character_code;
		msg_bar[6] = 0;
		auxdisplay_write(dev, msg_bar, 6);
		k_msleep(1000);
		auxdisplay_clear(dev);

		for (uint8_t i = 0x30; i < 0x36; i++) {
			auxdisplay_write(dev, &i, 1);
			k_msleep(250);
		}
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_RELATIVE, -1, 0);
		auxdisplay_write(dev, "6", 1);
		for (uint8_t i = 0x37; i < 0x3A; i++) {
			auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_RELATIVE, -2, 0);
			auxdisplay_write(dev, &i, 1);
			k_msleep(250);
		}
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_RELATIVE, -2, 0);
		auxdisplay_write(dev, " ", 1);
		k_msleep(500);
		auxdisplay_clear(dev);

		for (int i = 0; i < 12; i++) {
			auxdisplay_custom_indicator_set(dev, i, true);
			k_msleep(250);
		}
		k_msleep(500);
		auxdisplay_clear(dev);

		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_1, strlen(msg_alpha_num_1));
		k_msleep(1000);
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_2, strlen(msg_alpha_num_2));
		k_msleep(1000);
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_3, strlen(msg_alpha_num_3));
		k_msleep(1000);
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_4, strlen(msg_alpha_num_4));
		k_msleep(1000);
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_5, strlen(msg_alpha_num_5));
		k_msleep(1000);
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (const uint8_t *)msg_alpha_num_6, strlen(msg_alpha_num_6));
		k_msleep(1000);
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (const uint8_t *)msg_unit_prefixes,
				 strlen(msg_unit_prefixes));
		k_msleep(1000);
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (const uint8_t *)msg_operators, strlen(msg_operators));
		k_msleep(1000);
		auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
		auxdisplay_write(dev, (const uint8_t *)msg_symbols, strlen(msg_symbols));
		k_msleep(1000);
		auxdisplay_clear(dev);

		for (int i = 0; i < 3; i++) {
			auxdisplay_clear(dev);
			auxdisplay_write(dev, (const uint8_t *)msg_dot, strlen(msg_dot));
			k_msleep(500);

			auxdisplay_clear(dev);
			auxdisplay_write(dev, (const uint8_t *)msg_double_dot,
					 strlen(msg_double_dot));
			k_msleep(500);

			auxdisplay_clear(dev);
			auxdisplay_write(dev, (const uint8_t *)msg_dot, strlen(msg_dot));
			auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
			auxdisplay_write(dev, (const uint8_t *)msg_double_dot,
					 strlen(msg_double_dot));
			k_msleep(500);
		}
		k_msleep(500);
		auxdisplay_clear(dev);

		/* Bars: from 0 to 15 */
		msg_bar[0] = 'B';
		msg_bar[1] = 'A';
		msg_bar[2] = 'R';
		msg_bar[3] = 'S';
		msg_bar[6] = 0;
		for (int i = 0; i < 16; i++) {
			msg_bar[4] = i > 9 ? '1' : ' ';
			msg_bar[5] = i > 9 ? '0' + i - 10 : '0' + i;
			auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
			auxdisplay_write(dev, (uint8_t *)msg_bar, strlen(msg_bar));
			/* set bar indicators in function of i */
			auxdisplay_custom_indicator_set(dev, 12 - 4, i & 0x1);
			auxdisplay_custom_indicator_set(dev, 12 - 3, i & 0x2);
			auxdisplay_custom_indicator_set(dev, 12 - 2, i & 0x4);
			auxdisplay_custom_indicator_set(dev, 12 - 1, i & 0x8);
			k_msleep(500);
		}
		k_msleep(500);
		auxdisplay_clear(dev);

		/* Bars: from 0% to 100%, step 25% */
		msg_bar[3] = custom_character_degree.character_code;
		msg_bar[4] = '/';
		msg_bar[5] = custom_character_low_ring.character_code;
		msg_bar[6] = 0;
		/* TODO: write Degree Slash Lowring in 3, 4 , 5 */
		for (int i = 0; i < 5; i++) {
			snprintf(msg_bar, FIXED_TEXT_MESSAGE_SIZE, "%3d", i * 25);
			msg_bar[3] = custom_character_degree.character_code;
			auxdisplay_cursor_position_set(dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
			auxdisplay_write(dev, (uint8_t *)msg_bar, strlen(msg_bar));
			/* set bar indicators */
			if (i) {
				auxdisplay_custom_indicator_set(dev, 12 - 5 + i, true);
			}
			k_msleep(500);
		}
		k_msleep(500);
		auxdisplay_clear(dev);
	}

	return 0;
}
