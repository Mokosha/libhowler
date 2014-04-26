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

#include "howler.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>


/*******************************************************************************
 *
 * Internal types
 *
 ******************************************************************************/

typedef unsigned char bank_location[2];

/*******************************************************************************
 *
 * USB Command constants
 *
 ******************************************************************************/

static const unsigned char CMD_HOWLER_ID = 0xCE;
static const unsigned char CMD_SET_RGB_LED = 0x1;
static const unsigned char CMD_SET_INDIVIDUAL_LED = 0x2;
static const unsigned char CMD_SET_INPUT = 0x3;
static const unsigned char CMD_GET_INPUT = 0x4;
static const unsigned char CMD_SET_DEFAULT = 0x5;
static const unsigned char CMD_SET_GLOBAL_BRIGHTNESS = 0x6;
static const unsigned char CMD_SET_RGB_LED_DEFAULT = 0x7;
static const unsigned char CMD_GET_RGB_LED = 0x8;
static const unsigned char CMD_SET_RGB_LED_BANK = 0x9;
static const unsigned char CMD_GET_FW_REV = 0xA0;
static const unsigned char CMD_GET_ACCEL_DATA = 0xAC;

/*******************************************************************************
 *
 *  Static functions
 *
 *******************************************************************************
 */

static bank_location howler_button_to_bank[HOWLER_NUM_BUTTONS][3] = {
  /* Button 1 */
  { /* Red */ { /* Bank 0 */ 0, /* Location 1 */ 1 },
    /* Green */ { /* Bank 2 */ 2, /* Location 1 */ 1 },
    /* Blue */ { /* Bank 4 */ 4, /* Location 1 */ 1 },
  },

  /* Button 2 */
  { { 0, 2 }, { 2, 2 }, { 4, 2 } },

  /* ... etc ... */
  { { 0, 3 }, { 2, 3 }, { 4, 3 } },
  { { 0, 4 }, { 2, 4 }, { 4, 4 } },
  { { 0, 5 }, { 2, 5 }, { 4, 5 } },
  { { 0, 6 }, { 2, 6 }, { 4, 6 } },
  { { 0, 7 }, { 2, 7 }, { 4, 7 } },
  { { 1, 0 }, { 3, 0 }, { 5, 0 } },
  { { 1, 1 }, { 3, 1 }, { 5, 1 } },
  { { 1, 2 }, { 3, 2 }, { 5, 2 } },
  { { 1, 3 }, { 3, 3 }, { 5, 3 } },
  { { 1, 4 }, { 3, 4 }, { 5, 4 } },
  { { 1, 5 }, { 3, 5 }, { 5, 5 } },
  { { 0, 14 }, { 2, 14 }, { 4, 14 } },
  { { 0, 13 }, { 2, 13 }, { 4, 13 } },
  { { 0, 12 }, { 2, 12 }, { 4, 12 } },
  { { 0, 11 }, { 2, 11 }, { 4, 11 } },
  { { 0, 10 }, { 2, 10 }, { 4, 10 } },
  { { 0, 9 }, { 2, 9 }, { 4, 9 } },
  { { 0, 8 }, { 2, 8 }, { 4, 8 } },
  { { 1, 15 }, { 3, 15 }, { 5, 15 } },
  { { 1, 14 }, { 3, 14 }, { 5, 14 } },
  { { 1, 13 }, { 3, 13 }, { 5, 13 } },
  { { 1, 12 }, { 3, 12 }, { 5, 12 } },
  { { 1, 11 }, { 3, 11 }, { 5, 11 } },
  { { 1, 10 }, { 3, 10 }, { 5, 10 } },
};

static bank_location howler_joystick_to_bank[HOWLER_NUM_JOYSTICKS][3] = {
  { { 0, 0 }, { 2, 0 }, { 4, 0 } },
  { { 0, 15 }, { 2, 15 }, { 4, 15 } },
  { { 1, 6 }, { 3, 6 }, { 5, 6 } },
  { { 1, 9 }, { 3, 9 }, { 5, 9 } },
};

static bank_location howler_hp_led_to_bank[HOWLER_NUM_HIGH_POWER_LEDS][3]
= {
  { { 1, 7 }, { 3, 7 }, { 5, 7 } },
  { { 1, 8 }, { 3, 8 }, { 5, 8 } },
};

/* Control LEDs
 * LEDs are indexed according to the following scheme:
 * [0, 3] - Joystick LEDs 1-4, respectively
 * [4, 30] - Button LEDs 1-26, respectively
 * [31, 32] - High powered LEDs
 */

/* Sets a given LED to the proper intensity value */
static int howler_set_led_channel(howler_device *dev,
                                  unsigned char index,
                                  howler_led_channel_name channel,
                                  howler_led_channel value) {
  if(channel >= 85) {
    fprintf(stderr, "ERROR: howler_set_led invalid index: %d\n", index);
    return -1;
  }

  unsigned char cmd_buf[24];
  memset(cmd_buf, 0, sizeof(cmd_buf));

  cmd_buf[0] = CMD_HOWLER_ID;
  cmd_buf[1] = CMD_SET_INDIVIDUAL_LED;
  cmd_buf[2] = 3*index + (unsigned char)channel;
  cmd_buf[3] = value;

  return howler_sendrcv(dev, cmd_buf, NULL);
}

/* Sets a given LED to the proper RGB value */
static int howler_set_led(howler_device *dev,
                          unsigned char index,
                          howler_led led) {
  unsigned char cmd_buf[24];
  memset(cmd_buf, 0, sizeof(cmd_buf));

  cmd_buf[0] = CMD_HOWLER_ID;
  cmd_buf[1] = CMD_SET_RGB_LED;
  cmd_buf[2] = index;
  cmd_buf[3] = led.red;
  cmd_buf[4] = led.green;
  cmd_buf[5] = led.blue;

  return howler_sendrcv(dev, cmd_buf, NULL);
}

static int howler_set_led_bank(howler_device *dev, unsigned char index,
                               howler_led_bank *bank) {
  if(index >= 6) {
    fprintf(stderr, "ERROR: howler_set_led_bank invalid index: %d\n", index);
    return -1;
  }

  unsigned char cmd_buf[24];
  memset(cmd_buf, 0, sizeof(cmd_buf));

  cmd_buf[0] = CMD_HOWLER_ID;
  cmd_buf[1] = CMD_SET_RGB_LED_BANK;
  cmd_buf[2] = index;

  assert((sizeof(cmd_buf) - 3) > sizeof(*bank));
  memcpy(cmd_buf + 3, bank, sizeof(*bank));

  return howler_sendrcv(dev, cmd_buf, NULL);
}

/* Gets the LED values for a given index */
static int howler_get_led(howler_led *out,
                          howler_device *dev,
                          unsigned char index) {
  if(index >= HOWLER_NUM_LEDS) {
    fprintf(stderr, "ERROR: howler_get_led invalid index: %d\n", index);
    return -1;
  }

  unsigned char cmd_buf[24];
  memset(cmd_buf, 0, sizeof(cmd_buf));

  unsigned char output[24];
  memset(output, 0, sizeof(output));

  cmd_buf[0] = CMD_HOWLER_ID;
  cmd_buf[1] = CMD_GET_RGB_LED;
  cmd_buf[2] = index;

  if(howler_sendrcv(dev, cmd_buf, output) < 0) {
    return -1;
  }

  if(output[0] != CMD_HOWLER_ID || output[1] != CMD_GET_RGB_LED) {
    return -2;
  }

  out->red = output[2];
  out->green = output[3];
  out->blue = output[4];
  return 0;
}

/* Sets the LED banks for the given device */
static int update_led_bank(howler_device *dev, bank_location loc, unsigned char value) {
  unsigned char bank = loc[0];
  unsigned char led = loc[1];
  if(dev->led_banks[bank][led] == value) {
    return 0;
  }

  dev->led_banks[bank][led] = value;
  return howler_set_led_bank(dev, bank, &(dev->led_banks[bank]));
}

/*******************************************************************************
 *
 * USB Command constants
 * 
 ******************************************************************************/

size_t howler_get_num_connected(howler_context *ctx) {
  return ctx->nDevices;
}

howler_device *howler_get_device(howler_context *ctx, unsigned int device_index) {
  if(!ctx) { return NULL; }
  if(device_index >= howler_get_num_connected(ctx)) { return NULL; }
  return ctx->devices + device_index;
}

int howler_get_device_version(howler_device *dev, char *dst,
                              size_t dst_size, size_t *dst_len) {
  // Setup the commands
  unsigned char cmd_buf[24];
  memset(cmd_buf, 0, sizeof(cmd_buf));

  unsigned char output[24];
  memset(output, 0, sizeof(output));

  cmd_buf[0] = CMD_HOWLER_ID;
  cmd_buf[1] = CMD_GET_FW_REV;

  int err = howler_sendrcv(dev, cmd_buf, output);
  if(err < 0 || output[0] != CMD_HOWLER_ID || output[1] != CMD_GET_FW_REV) {
    return 0;
  }


  float version = (float)(output[2]) + 0.001 * (float)(output[3]);
  snprintf(dst, dst_size, "%.3f", version);
  dst[dst_size - 1] = '\0';

  if(dst_len) {
    *dst_len = strlen(dst) + 1;
  }

  return err;
}

/* Sets the RGB LED value of the given button
 * Buttons are numbered from 1 to 26 */
int howler_set_button_led(howler_device *dev,
                          unsigned char button,
                          howler_led led) {
#if 0
  int err = 0;
  err = err || howler_set_button_led_channel(dev, button, 0, led.red);
  err = err || howler_set_button_led_channel(dev, button, 1, led.green);
  err = err || howler_set_button_led_channel(dev, button, 2, led.blue);
  return -err;
#else
  unsigned char button_offset = 4;
  return howler_set_led(dev, button + button_offset - 1, led);
#endif
}

/* Sets the LED value of the specific channel for the button
 * Buttons are numbered from 1 to 26 */
int howler_set_button_led_channel(howler_device *dev,
                                  unsigned char button,
                                  howler_led_channel_name channel,
                                  howler_led_channel value) {
#if 0
  int button_index = (int)button - 1;
  if(button_index < 0 || button_index >= HOWLER_NUM_BUTTONS) {
    fprintf(stderr, "ERROR: howler_set_button_led invalid button index: %d\n",
            button);
    return -1;
  }

  return update_led_bank(dev, howler_button_to_bank[button_index][channel], value);
#else
  unsigned char button_offset = 4;
  return howler_set_led_channel(dev, button + button_offset - 1, channel, value);
#endif
}

/* Gets the LED value of the specific channel for the button
 * Buttons are numbered from 1 to 26 */
int howler_get_button_led(howler_led *out,
                                  howler_device *dev,
                                  unsigned char button) {
  int button_index = (int)button - 1;
  if(button_index < 0 || button_index >= HOWLER_NUM_BUTTONS) {
    fprintf(stderr, "ERROR: howler_set_button_led invalid button index: %d\n",
            button);
    return -1;
  }

  int button_offset = 4;
  return howler_get_led(out, dev, button_offset + button_index);
}

/* Sets the RGB LED value of the given joystick
 * Joysticks are numbered from 1 to 4 */
int howler_set_joystick_led(howler_device *dev,
                            unsigned char joystick,
                            howler_led led) {
  int err = 0;
  err = err || howler_set_button_led_channel(dev, joystick, 0, led.red);
  err = err || howler_set_button_led_channel(dev, joystick, 1, led.green);
  err = err || howler_set_button_led_channel(dev, joystick, 2, led.blue);
  return -err;
}

/* Sets the LED value of the specific channel for the given joystick
 * Joysticks are numbered from 1 to 4 */
int howler_set_joystick_led_channel(howler_device *dev,
                                    unsigned char joystick,
                                    howler_led_channel_name channel,
                                    howler_led_channel value) {
  int joystick_index = (int)joystick - 1;
  if(joystick_index < 0 || joystick_index >= HOWLER_NUM_JOYSTICKS) {
    fprintf(stderr, "ERROR: howler_set_joystick_led invalid joystick index: %d\n",
            joystick);
    return -1;
  }

  return update_led_bank(dev, howler_joystick_to_bank[joystick_index][channel], value);
}

/* Sets the RGB LED value of the given high powered LED controller
 * High powere LED controllers are numbered from 1 to 2 */
int howler_set_high_power_led(howler_device *dev,
                              unsigned char index,
                              howler_led led) {
  int err = 0;
  err = err || howler_set_button_led_channel(dev, index, 0, led.red);
  err = err || howler_set_button_led_channel(dev, index, 1, led.green);
  err = err || howler_set_button_led_channel(dev, index, 2, led.blue);
  return -err;
}

/* Sets the LED value of the specific channel for the given high powered LED
 * High powere LED controllers are numbered from 1 to 2 */
int howler_set_high_power_led_channel(howler_device *dev,
                                      unsigned char index,
                                      howler_led_channel_name channel,
                                      howler_led_channel value) {
  int high_power_index = (int)index - 1;
  if(high_power_index < 0 || high_power_index >= HOWLER_NUM_HIGH_POWER_LEDS) {
    fprintf(stderr, "ERROR: howler_set_high_power_led invalid high_power index: %d\n",
            index);
    return -1;
  }

  return update_led_bank(dev, howler_hp_led_to_bank[high_power_index][channel], value);
}

