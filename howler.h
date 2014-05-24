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
  void *usb_handle;
  howler_led_bank led_banks[6];
} howler_device;

extern unsigned char howler_button_to_bank[HOWLER_NUM_BUTTONS][3][2];
extern unsigned char howler_joystick_to_bank[HOWLER_NUM_JOYSTICKS][3][2];
extern unsigned char howler_hp_led_to_bank[HOWLER_NUM_HIGH_POWER_LEDS][3][2];

typedef void (*howler_button_callback)(int button, void *user_data);

typedef struct {
  void *usb_ctx;
  void *polling;
  size_t nDevices;
  howler_device *devices;

  int exitFlag;
  howler_button_callback key_down_callback;
  howler_button_callback key_up_callback;
} howler_context;

static const int HOWLER_SUCCESS = 0;
static const int HOWLER_ERROR_INVALID_PTR = -1;
static const int HOWLER_ERROR_LIBUSB_CONTEXT_ERROR = -2;
static const int HOWLER_ERROR_LIBUSB_DEVICE_LIST_ERROR = -3;
static const int HOWLER_ERROR_INVALID_PARAMS = -4;

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

/* Internal function used to send and receive messages from the howler device.
 * You should never need to call this function directly.
 */
int howler_sendrcv(howler_device *dev, unsigned char *cmd_buf,
                   unsigned char *output);

/* Returns the number of connected Howler devices */
size_t howler_get_num_connected(howler_context *ctx);

/* Returns a pointer to the device located at index 'device_index'. This pointer
 * will be NULL if:
 *   device_index >= howler_get_num_connected(ctx)
 */
howler_device *howler_get_device(howler_context *ctx,
                                 unsigned int device_index);

/*******************************************************************************
 *
 * LED Commands
 *
 ******************************************************************************/

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

/*******************************************************************************
 *
 * Interface Controls
 *
 ******************************************************************************/

typedef enum {
  eHowlerKeyScanCode_A          = 0x04,
  eHowlerKeyScanCode_B          = 0x05,
  eHowlerKeyScanCode_C          = 0x06,
  eHowlerKeyScanCode_D          = 0x07,
  eHowlerKeyScanCode_E          = 0x08,
  eHowlerKeyScanCode_F          = 0x09,
  eHowlerKeyScanCode_G          = 0x0A,
  eHowlerKeyScanCode_H          = 0x0B,
  eHowlerKeyScanCode_I          = 0x0C,
  eHowlerKeyScanCode_J          = 0x0D,
  eHowlerKeyScanCode_K          = 0x0E,
  eHowlerKeyScanCode_L          = 0x0F,
  eHowlerKeyScanCode_M          = 0x10,
  eHowlerKeyScanCode_N          = 0x11,
  eHowlerKeyScanCode_O          = 0x12,
  eHowlerKeyScanCode_P          = 0x13,
  eHowlerKeyScanCode_Q          = 0x14,
  eHowlerKeyScanCode_R          = 0x15,
  eHowlerKeyScanCode_S          = 0x16,
  eHowlerKeyScanCode_T          = 0x17,
  eHowlerKeyScanCode_U          = 0x18,
  eHowlerKeyScanCode_V          = 0x19,
  eHowlerKeyScanCode_W          = 0x1A,
  eHowlerKeyScanCode_X          = 0x1B,
  eHowlerKeyScanCode_Y          = 0x1C,
  eHowlerKeyScanCode_Z          = 0x1D,
  eHowlerKeyScanCode_1          = 30,
  eHowlerKeyScanCode_2          = 31,
  eHowlerKeyScanCode_3          = 32,
  eHowlerKeyScanCode_4          = 33,
  eHowlerKeyScanCode_5          = 34,
  eHowlerKeyScanCode_6          = 35,
  eHowlerKeyScanCode_7          = 36,
  eHowlerKeyScanCode_8          = 37,
  eHowlerKeyScanCode_9          = 38,
  eHowlerKeyScanCode_0          = 39,
  eHowlerKeyScanCode_ENTER      = 40,
  eHowlerKeyScanCode_ESCAPE     = 41,
  eHowlerKeyScanCode_BACKSPACE  = 42,
  eHowlerKeyScanCode_TAB        = 43,
  eHowlerKeyScanCode_SPACEBAR   = 44,
  eHowlerKeyScanCode_UNDERSCORE = 45,
  eHowlerKeyScanCode_PLUS       = 46,
  eHowlerKeyScanCode_OPEN_BRACKET = 47, // {
  eHowlerKeyScanCode_CLOSE_BRACKET = 48, // }
  eHowlerKeyScanCode_BACKSLASH  = 49,
  eHowlerKeyScanCode_ASH        = 50, // # ~
  eHowlerKeyScanCode_COLON      = 51, // ; :
  eHowlerKeyScanCode_QUOTE      = 52, // ' "
  eHowlerKeyScanCode_TILDE      = 53,
  eHowlerKeyScanCode_COMMA      = 54,
  eHowlerKeyScanCode_DOT        = 55,
  eHowlerKeyScanCode_SLASH      = 56,
  eHowlerKeyScanCode_CAPS_LOCK  = 57,
  eHowlerKeyScanCode_F1         = 58,
  eHowlerKeyScanCode_F2         = 59,
  eHowlerKeyScanCode_F3         = 60,
  eHowlerKeyScanCode_F4         = 61,
  eHowlerKeyScanCode_F5         = 62,
  eHowlerKeyScanCode_F6         = 63,
  eHowlerKeyScanCode_F7         = 64,
  eHowlerKeyScanCode_F8         = 65,
  eHowlerKeyScanCode_F9         = 66,
  eHowlerKeyScanCode_F10        = 67,
  eHowlerKeyScanCode_F11        = 68,
  eHowlerKeyScanCode_F12        = 69,
  eHowlerKeyScanCode_PRINTSCREEN = 70,
  eHowlerKeyScanCode_SCROLL_LOCK = 71,
  eHowlerKeyScanCode_PAUSE      = 72,
  eHowlerKeyScanCode_INSERT     = 73,
  eHowlerKeyScanCode_HOME       = 74,
  eHowlerKeyScanCode_PAGEUP     = 75,
  eHowlerKeyScanCode_DELETE     = 76,
  eHowlerKeyScanCode_END        = 77,
  eHowlerKeyScanCode_PAGEDOWN   = 78,
  eHowlerKeyScanCode_RIGHT      = 79,
  eHowlerKeyScanCode_LEFT       = 80,
  eHowlerKeyScanCode_DOWN       = 81,
  eHowlerKeyScanCode_UP         = 82,
  eHowlerKeyScanCode_KEYPAD_NUM_LOCK = 83,
  eHowlerKeyScanCode_KEYPAD_DIVIDE   = 84,
  eHowlerKeyScanCode_KEYPAD_AT       = 85,
  eHowlerKeyScanCode_KEYPAD_MULTIPLY = 85,
  eHowlerKeyScanCode_KEYPAD_MINUS    = 86,
  eHowlerKeyScanCode_KEYPAD_PLUS     = 87,
  eHowlerKeyScanCode_KEYPAD_ENTER    = 88,
  eHowlerKeyScanCode_KEYPAD_1        = 89,
  eHowlerKeyScanCode_KEYPAD_2        = 90,
  eHowlerKeyScanCode_KEYPAD_3        = 91,
  eHowlerKeyScanCode_KEYPAD_4        = 92,
  eHowlerKeyScanCode_KEYPAD_5        = 93,
  eHowlerKeyScanCode_KEYPAD_6        = 94,
  eHowlerKeyScanCode_KEYPAD_7        = 95,
  eHowlerKeyScanCode_KEYPAD_8        = 96,
  eHowlerKeyScanCode_KEYPAD_9        = 97,
  eHowlerKeyScanCode_KEYPAD_0        = 98,

  eHowlerKeyScanCode_FIRST = eHowlerKeyScanCode_A,
  eHowlerKeyScanCode_LAST = eHowlerKeyScanCode_KEYPAD_0,
} howler_key_scan_code;

#define NUM_HOWLER_KEY_SCAN_CODES 96

typedef enum {
  eHowlerKeyModifier_None       = 0x00,
  eHowlerKeyModifier_LeftShift  = 0x01,
  eHowlerKeyModifier_RightShift = 0x02,
  eHowlerKeyModifier_LeftCtrl   = 0x04,
  eHowlerKeyModifier_RightCtrl  = 0x08,
  eHowlerKeyModifier_LeftAlt    = 0x10,
  eHowlerKeyModifier_RightAlt   = 0x20,
  eHowlerKeyModifier_LeftUI     = 0x40,
  eHowlerKeyModifier_RightUI    = 0x80
} howler_key_modifiers;

typedef enum {
  eHowlerInput_Joystick1Up = 0x00,
  eHowlerInput_Joystick1Down = 0x01,
  eHowlerInput_Joystick1Left = 0x02,
  eHowlerInput_Joystick1Right = 0x03,
  eHowlerInput_Joystick2Up = 0x04,
  eHowlerInput_Joystick2Down = 0x05,
  eHowlerInput_Joystick2Left = 0x06,
  eHowlerInput_Joystick2Right = 0x07,
  eHowlerInput_Joystick3Up = 0x08,
  eHowlerInput_Joystick3Down = 0x09,
  eHowlerInput_Joystick3Left = 0x0A,
  eHowlerInput_Joystick3Right = 0x0B,
  eHowlerInput_Joystick4Up = 0x0C,
  eHowlerInput_Joystick4Down = 0x0D,
  eHowlerInput_Joystick4Left = 0x0E,
  eHowlerInput_Joystick4Right = 0x0F,
  eHowlerInput_Button1  = 0x10,
  eHowlerInput_Button2  = 0x11,
  eHowlerInput_Button3  = 0x12,
  eHowlerInput_Button4  = 0x13,
  eHowlerInput_Button5  = 0x14,
  eHowlerInput_Button6  = 0x15,
  eHowlerInput_Button7  = 0x16,
  eHowlerInput_Button8  = 0x17,
  eHowlerInput_Button9  = 0x18,
  eHowlerInput_Button10 = 0x19,
  eHowlerInput_Button11 = 0x1A,
  eHowlerInput_Button12 = 0x1B,
  eHowlerInput_Button13 = 0x1C,
  eHowlerInput_Button14 = 0x1D,
  eHowlerInput_Button15 = 0x1E,
  eHowlerInput_Button16 = 0x1F,
  eHowlerInput_Button17 = 0x20,
  eHowlerInput_Button18 = 0x21,
  eHowlerInput_Button19 = 0x22,
  eHowlerInput_Button20 = 0x23,
  eHowlerInput_Button21 = 0x24,
  eHowlerInput_Button22 = 0x25,
  eHowlerInput_Button23 = 0x26,
  eHowlerInput_Button24 = 0x27,
  eHowlerInput_Button25 = 0x28,
  eHowlerInput_Button26 = 0x29,
  eHowlerInput_AccelerometerX = 0x2A,
  eHowlerInput_AccelerometerY = 0x2B,
  eHowlerInput_AccelerometerZ = 0x2C,

  eHowlerInput_FIRST = eHowlerInput_Joystick1Up,
  eHowlerInput_LAST = eHowlerInput_AccelerometerZ
} howler_input;

static const size_t NUM_HOWLER_INPUTS = eHowlerInput_LAST + 1;

/* Maps a given howler input to the corresponding keyboard scan key. The howler
 * should already be installed as a HID device, and hence acts as a keyboard.
 * This function simply allows applications to modify what keys are pressed on
 * the virtual keyboard with respect to buttons on the Howler. */
int howler_set_input_keyboard(howler_device *dev, howler_input ipt,
                              howler_key_scan_code code,
                              howler_key_modifiers modifiers);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // __HOWLER_LIB_H__
