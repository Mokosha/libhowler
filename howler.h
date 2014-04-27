/* The MIT License (MIT)
 *
 * Copyright (c) 2014 Pavel Krajcevski
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __HOWLER_LIB_H__
#define __HOWLER_LIB_H__

#include <stdlib.h>
#include <libusb.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 *
 * USB Command constants
 *
 ******************************************************************************/

#define CMD_HOWLER_ID 0xCE
#define CMD_SET_RGB_LED 0x1
#define CMD_SET_INDIVIDUAL_LED 0x2
#define CMD_SET_INPUT 0x3
#define CMD_GET_INPUT 0x4
#define CMD_SET_DEFAULT 0x5
#define CMD_SET_GLOBAL_BRIGHTNESS 0x6
#define CMD_SET_RGB_LED_DEFAULT 0x7
#define CMD_GET_RGB_LED 0x8
#define CMD_SET_RGB_LED_BANK 0x9
#define CMD_GET_FW_REV 0xA0
#define CMD_GET_ACCEL_DATA 0xAC

#define HOWLER_NUM_BUTTONS 26
#define HOWLER_NUM_JOYSTICKS 4
#define HOWLER_NUM_HIGH_POWER_LEDS 2
#define HOWLER_NUM_LEDS \
  (HOWLER_NUM_BUTTONS + HOWLER_NUM_JOYSTICKS + HOWLER_NUM_HIGH_POWER_LEDS)

typedef unsigned char howler_led_channel;
typedef union {
  struct {
    howler_led_channel red;
    howler_led_channel green;
    howler_led_channel blue;
  };
  howler_led_channel channels[3];
} howler_led;

typedef enum {
  HOWLER_LED_CHANNEL_RED = 0,
  HOWLER_LED_CHANNEL_GREEN,
  HOWLER_LED_CHANNEL_BLUE
} howler_led_channel_name;

typedef howler_led_channel howler_led_bank[16];
typedef struct {
  libusb_device *usb_device;
  libusb_device_handle *usb_handle;
  howler_led_bank led_banks[6];
} howler_device;

typedef struct {
  libusb_context *usb_ctx;
  size_t nDevices;
  howler_device *devices;
} howler_context;

static const int HOWLER_SUCCESS = 0;
static const int HOWLER_ERROR_INVALID_PTR = -1;
static const int HOWLER_ERROR_LIBUSB_CONTEXT_ERROR = -2;
static const int HOWLER_ERROR_LIBUSB_DEVICE_LIST_ERROR = -3;

/* Constant variables */
static const unsigned short HOWLER_VENDOR_ID = 0x3EB;

#define MAX_HOWLER_DEVICE_IDS 4
static const unsigned short HOWLER_DEVICE_ID[MAX_HOWLER_DEVICE_IDS] =
  { 0x6800, 0x6801, 0x6802, 0x6803 };

/* Initialize the Howler context. This context interfaces with all of the
 * identifiable howler controllers. It first makes sure to initialize libusb
 * in order to send commands. It returns 0 on success, and an error otherwise
 */
int howler_init(howler_context **);
void howler_destroy(howler_context *);

/* Internal function used to send and receive messages from the howler device. You
 * should never need to call this function directly.
 */
int howler_sendrcv(howler_device *dev, unsigned char *cmd_buf, unsigned char *output);

/* Returns the number of connected Howler devices */
size_t howler_get_num_connected(howler_context *ctx);

/* Returns a pointer to the device located at index 'device_index'. This pointer
 * will be NULL if:
 *   device_index >= howler_get_num_connected(ctx)
 */
howler_device *howler_get_device(howler_context *ctx, unsigned int device_index);

/* Returns the version string for the associated device.
 * dst - A buffer to take the version string
 * dst_size - The size of dst in bytes
 * dst_len - The actual length of dst
 */
int howler_get_device_version(howler_device *dev, char *dst,
                              size_t dst_size, size_t *dst_len);

int howler_set_global_brightness(howler_device *dev, howler_led_channel level);

/* Sets the RGB LED value of the given button
 * Buttons are numbered from 1 to 26 */
int howler_set_button_led(howler_device *dev,
                          unsigned char button,
                          howler_led led);

/* Gets the RGB LED value of the given button
 * Buttons are numbered from 1 to 26 */
int howler_get_button_led(howler_led *out,
                          howler_device *dev,
                          unsigned char button);

/* Sets the LED value of the specific channel for the button
 * Buttons are numbered from 1 to 26 */
int howler_set_button_led_channel(howler_device *dev,
                                  unsigned char button,
                                  howler_led_channel_name channel,
                                  howler_led_channel value);

/* Sets the RGB LED value of the given joystick
 * Joysticks are numbered from 1 to 4 */
int howler_set_joystick_led(howler_device *dev,
                            unsigned char joystick,
                            howler_led led);

/* Sets the LED value of the specific channel for the given joystick
 * Joysticks are numbered from 1 to 4 */
int howler_set_joystick_led_channel(howler_device *dev,
                                    unsigned char joystick,
                                    howler_led_channel_name channel,
                                    howler_led_channel value);

/* Gets the LED value of the specific channel for the joystick
 * Joysticks are numbered from 1 to 4 */
int howler_get_joystick_led(howler_led *out,
                            howler_device *dev,
                            unsigned char joystick);

/* Sets the RGB LED value of the given high powered LED controller
 * High powere LED controllers are numbered from 1 to 2 */
int howler_set_high_power_led(howler_device *dev,
                              unsigned char index,
                              howler_led led);

/* Sets the LED value of the specific channel for the given high powered LED
 * High powere LED controllers are numbered from 1 to 2 */
int howler_set_high_power_led_channel(howler_device *dev,
                                      unsigned char index,
                                      howler_led_channel_name channel,
                                      howler_led_channel value);

/* Gets the LED value of the specific channel for the high powered LED
 * High powered LEDs are numbered from 1 to 2 */
int howler_get_high_power_led(howler_led *out,
                              howler_device *dev,
                              unsigned char high_power_led);

extern unsigned char howler_button_to_bank[HOWLER_NUM_BUTTONS][3][2];
extern unsigned char howler_joystick_to_bank[HOWLER_NUM_JOYSTICKS][3][2];
extern unsigned char howler_hp_led_to_bank[HOWLER_NUM_HIGH_POWER_LEDS][3][2];
#ifdef __cplusplus
} // extern "C"
#endif

#endif  // __HOWLER_LIB_H__
