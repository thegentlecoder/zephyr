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
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(auxdisplay_14seg_qemu, LOG_LEVEL_DBG);

struct auxdisplay_panel_config {
	struct auxdisplay_capabilities capabilities;
	const bool rotated;
	const uint8_t segment_type;
	const uint16_t *digits; /* Digits coding array. */
	const uint16_t *digits_rotated_180;
	const uint16_t *letters_upper; /* Upper Letter coding array. */
	const uint16_t *letters_upper_rotated_180;
	const uint16_t *letters_lower; /* Lower Letter coding array. */
	const uint16_t *letters_lower_rotated_180;
	/*
	 * Pin/com configurations of the segments/indicators read from the
	 * panel overlay configuration.
	 */
	const uint8_t *segment_pins;
	const uint8_t *segment_coms;
	const uint8_t num_indicators;
	const uint8_t *indicator_pins;       /* Optional. NULL if DT property absent */
	const uint8_t *indicator_coms;       /* Optional. NULL if DT property absent */
	const uint8_t *display_indicators;   /* Optional. NULL if DT property absent */
	const uint8_t *col_indicators;       /* Optional. NULL if DT property absent */
	const uint8_t *upper_dot_indicators; /* Optional. NULL if DT property absent */
	const uint8_t *lower_dot_indicators; /* Optional. NULL if DT property absent */
};

struct auxdisplay_st_slcd_config {
	const struct stm32_pclken *pclken;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pincfg;

	const uint8_t *pin_list;
	const uint8_t pin_list_len;
	const uint8_t com_list_len;
	const uint8_t ram_buffer_size;

	const uint8_t custom_character_slot_count;
	const uint32_t *character_bit_list;
	const uint8_t *character_com_list;
	const uint32_t *indicator_bit_list;
	const uint8_t *indicator_com_list;

	const struct auxdisplay_panel_config *panel_config;
	const int position_count;
};

extern uint16_t
slcd_14seg_convert_ascii_char_to_14seg_pattern(const struct auxdisplay_panel_config *panel,
					       uint8_t ascii_char);

struct full_test_fixture {
	const struct device *display;
	const struct auxdisplay_panel_config *panel_config;
};

static void *suite_setup(void)
{
	static struct full_test_fixture context;

	context.display = DEVICE_DT_GET(DT_NODELABEL(auxdisplay_0));
	context.panel_config = ((const struct auxdisplay_st_slcd_config *)context.display->config)->panel_config;
	return &context;
}

ZTEST_SUITE(full_test, NULL, suite_setup, NULL, NULL, NULL);

ZTEST_F(full_test, test_display_ready)
{
	zassert_not_null(fixture->display, "Segment LCD device not instantiated");
	zassert_true(device_is_ready(fixture->display),
		     "Segment LCD device is not ready for operation");
	zassert_not_null(fixture->panel_config, "Segment LCD panel config not found");
}

ZTEST_F(full_test, test_auxdisplay_write_uppercase_a)
{
	const char t = 'A';
	int rc = auxdisplay_write(fixture->display, (const uint8_t *)&t, 1);
	zassert_equal(rc, 0, "La scrittura su auxdisplay ha fallito (%d)", rc);
}

ZTEST_F(full_test, test_auxdisplay_write_uppercase_z)
{
	const char t = 'Z';
	int rc = auxdisplay_write(fixture->display, (const uint8_t *)&t, 1);

	zassert_equal(rc, 0, "La scrittura su auxdisplay ha fallito (%d)", rc);
}

ZTEST_F(full_test, test_slcd_14seg_convert_ascii_char_to_14seg_pattern_uppercase_a)
{
	uint16_t pattern = slcd_14seg_convert_ascii_char_to_14seg_pattern(fixture->panel_config, 'A');

	zassert_equal(pattern, 0x477, "Pattern for letter 'A' is wrong (0x%x)", pattern);
}

ZTEST_F(full_test, test_slcd_14seg_convert_ascii_char_to_14seg_pattern_uppercase_z)
{
	uint16_t pattern = slcd_14seg_convert_ascii_char_to_14seg_pattern(fixture->panel_config, 'Z');

	zassert_equal(pattern, 0x2209, "Pattern for letter 'A' is wrong (0x%x)", pattern);
}
