/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Screen */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <stdio.h>

/* Buttons */
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/gpio.h>

/* accelerometer */
#include <zephyr/drivers/sensor.h>

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define BTN_A_NODE	DT_NODELABEL(buttona)
#define BTN_B_NODE	DT_NODELABEL(buttonb)
#define BTN_C_NODE	DT_NODELABEL(buttonc)
#if !DT_NODE_HAS_STATUS(BTN_A_NODE, okay) || !DT_NODE_HAS_STATUS(BTN_B_NODE, okay)  || !DT_NODE_HAS_STATUS(BTN_C_NODE, okay) 
#error "Unsupported board: button_a, b or c devicetree node(s) is/are not defined"
#endif

static const struct gpio_dt_spec buttona = GPIO_DT_SPEC_GET_OR(BTN_A_NODE, gpios, {0});
static const struct gpio_dt_spec buttonb = GPIO_DT_SPEC_GET_OR(BTN_B_NODE, gpios, {0});
static const struct gpio_dt_spec buttonc = GPIO_DT_SPEC_GET_OR(BTN_C_NODE, gpios, {0});

const struct device *const lsm6dso = DEVICE_DT_GET_ONE(st_lsm6dso);

void configure_button(const struct gpio_dt_spec *button)
{
	if (!device_is_ready(button->port)) {
		printk("Button device not ready: %s\n", button->port->name);
		return;
	}

	int ret = gpio_pin_configure_dt(button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button->port->name, button->pin);
	}

	printk("Success.  Configured %s pin %d\n", button->port->name, button->pin);
}

int init_screen(const struct device *display)
{
	uint16_t x_res;
	uint16_t y_res;
	uint16_t rows;
	uint8_t ppt;
	uint8_t font_width;
	uint8_t font_height;

	if (display_set_pixel_format(display, PIXEL_FORMAT_MONO10) != 0) {
		if (display_set_pixel_format(display, PIXEL_FORMAT_MONO01) != 0) {
			printf("Failed to set required pixel format");
			return 0;
		} else {
			printf("display_set...(MONO01)\n");
		}
	} else {
		printf("display_set....(MONO10)\n");
	}

	printf("Initialized %s\n", display->name);

	if (cfb_framebuffer_init(display)) {
		printf("Framebuffer initialization failed!\n");
		return 0;
	}


	cfb_framebuffer_clear(display, true);

	display_blanking_off(display);

	x_res = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	y_res = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGH);
	rows = cfb_get_display_parameter(display, CFB_DISPLAY_ROWS);
	ppt = cfb_get_display_parameter(display, CFB_DISPLAY_PPT);

	for (int idx = 0; idx < 42; idx++) {
		if (cfb_get_font_size(display, idx, &font_width, &font_height)) {
			break;
		}
		cfb_framebuffer_set_font(display, idx);
		printf("font width %d, font height %d\n",
		       font_width, font_height);
	}
	cfb_framebuffer_set_font(display, 0);


	printf("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       x_res,
	       y_res,
	       ppt,
	       rows,
	       cfb_get_display_parameter(display, CFB_DISPLAY_COLS));

	// cfb_framebuffer_invert(display);

	// cfb_set_kerning(display, 3);

	return 0;
}

void joy_feather(void)
{}

void get_accelerometer(double *x, double *y, double *z)
{
		struct sensor_value accel_xyz[3];

		if (sensor_sample_fetch(lsm6dso) < 0) {
			printf("LSM6DSO Sensor sample update error\n");
		}
		sensor_channel_get(lsm6dso, SENSOR_CHAN_ACCEL_XYZ, accel_xyz);

		printf(
		   "LSM6DSO: Acceleration (m.s-2): x: %.1f, y: %.1f, z: %.1f\n",
		   sensor_value_to_double(&accel_xyz[0]),
		   sensor_value_to_double(&accel_xyz[1]),
		   sensor_value_to_double(&accel_xyz[2]));

		*x = sensor_value_to_double(&accel_xyz[0]);
		*y = sensor_value_to_double(&accel_xyz[1]);
		*z = sensor_value_to_double(&accel_xyz[2]);
}	

int main(void)
{
	const struct device *display;
	char output_str[64];

	configure_button(&buttona);
	configure_button(&buttonb);
	configure_button(&buttonc);

	display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display)) {
		printf("Device %s not ready\n", display->name);
		return 0;
	}

	init_screen(display);

	if (!device_is_ready(lsm6dso)) {
		printk("%s: device not ready.\n", lsm6dso->name);
		// return 0;
	}


	while (1) {
		double acc_x, acc_y, acc_z;
		get_accelerometer(&acc_x, &acc_y, &acc_z);
		snprintf(output_str, 63,
		   "%4.1f %4.1f %4.1f        ",
		   acc_x, acc_y, acc_z);

		int vala = gpio_pin_get_dt(&buttona);
		int valb = gpio_pin_get_dt(&buttonb);
		int valc = gpio_pin_get_dt(&buttonc);

		printf("read keys: %d %d %d\n", vala, valb, valc);

		char key_str[64];
		snprintf(key_str, 63,
		   "%c %c %c 	  ",
		   vala ? 'A' : 'a',
		   valb ? 'B' : 'b',
		   valc ? 'C' : 'c');
		   
		// cfb_framebuffer_clear(display, true);
		if (cfb_print(display, key_str, 0, 0)) {
			printf("Failed to print a string\n");
			continue;
		}
		if (cfb_print(display, output_str, 0, 16)) {
			printf("Failed to print a string\n");
			continue;
		}
		
		// printf("i = %d\n", i);
		cfb_framebuffer_finalize(display);
		k_sleep(K_MSEC(50));
	}
	
	while (1) {
		double acc_x, acc_y, acc_z;
		get_accelerometer(&acc_x, &acc_y, &acc_z);
		snprintf(output_str, 63,
		   "%4.1f %4.1f %4.1f        ",
		   acc_x, acc_y, acc_z);

		int vala = gpio_pin_get_dt(&buttona);
		int valb = gpio_pin_get_dt(&buttonb);
		int valc = gpio_pin_get_dt(&buttonc);

		printf("read keys: %d %d %d\n", vala, valb, valc);

		char key_str[64];
		snprintf(key_str, 63,
		   "%c %c %c 	  ",
		   vala ? 'A' : 'a',
		   valb ? 'B' : 'b',
		   valc ? 'C' : 'c');
		   
		// cfb_framebuffer_clear(display, true);
		if (cfb_print(display, key_str, 0, 0)) {
			printf("Failed to print a string\n");
			continue;
		}
		if (cfb_print(display, output_str, 0, 16)) {
			printf("Failed to print a string\n");
			continue;
		}
		
		// printf("i = %d\n", i);
		cfb_framebuffer_finalize(display);
		k_sleep(K_MSEC(50));
	}
	return 0;
}
