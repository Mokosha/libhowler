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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "howler.h"

typedef enum {
  TYPE_BUTTON,
  TYPE_JOYSTICK,
  TYPE_HIGH_POWER_LED
} control_type;

typedef struct {
  int index;
  control_type type;
} control;

typedef int (*command_function)(howler_device *device, int cmd_idx, const char **argv, int argc);

static void print_version() {
  printf("HowlerCtl Version 0.0.1\n");
}

static void print_usage() {
  print_version();
  printf("Usage: howlerctl [DEVICE] COMMAND [OPTIONS]\n");
  printf("\n");
  printf("    DEVICE is a number from 0 to 3 that designates the\n");
  printf("    corresponding Howler device. The default is 0.\n");
  printf("\n");
  printf("    COMMAND is one of the following:\n");
  printf("        help\n");
  printf("        get-firmware\n");
  printf("        get-led [CONTROL]\n");
  printf("        set-led-channel CONTROL (red|green|blue) VALUE\n");
  printf("        set-led CONTROL RED GREEN BLUE\n");
  printf("        set-key INPUT KEY [MODIFIER[+MODIFIER[+...]]]\n");
  printf("\n");
  printf("    CONTROL is a string conforming to one of the following:\n");
  printf("        J1 - J4: Joystick 1 to Joystick 4\n");
  printf("        B1 - B26: Button 1 to Button 26\n");
  printf("        H1 - H2: High Power LED 1 or 2\n");
  printf("\n");
  printf("    INPUT is a string conforming to one of the following:\n");
  printf("        J[1-4][U|D|L|R]: Joystick number and direction (Up Down Left Right)\n");
  printf("        B1 - B26: Button 1 to Button 26\n");
  printf("\n");
  printf("    KEY is a character from a standard US keyboard\n");
  printf("        use the command 'howlerctl list-supported-keys' to print a list\n");
  printf("\n");
  printf("    MODIFIER is any of the following:\n");
  printf("        LSHIFT, RSHIFT, LCTRL, RCTRL, LALT, RALT, LUI, RUI\n");
}

static int get_firmware(howler_device *dev, char *buf, size_t bufSz) {
  if(howler_get_device_version(dev, buf, bufSz, NULL) < 0) {
    fprintf(stderr, "INTERNAL ERROR: Unable to get howler firmware version.\n");
    return -1;
  }
  return 0;
}

static int parse_device(const char *dev_str) {
  int device = 0;
  int num_read = sscanf(dev_str, "%d", &device);
  if(0 == num_read) {
    return -2;
  }

  if(device < 0 || device > 4) {
    return -1;
  }

  return device;
}

static int parse_byte(unsigned char *ret, const char *str, const char *name) {
  int ret_val = 0;
  unsigned long val;
  if(sscanf(str, "%lu", &val) < 1) {
    if(name) {
      fprintf(stderr, "Invalid value for %s: %s\n", name, str);
    } else {
      fprintf(stderr, "Invalid value: %s\n", str);
    }
    ret_val = -1;
  } else if(val > 255) {
    if(name) {
      fprintf(stderr, "Invalid value for %s: %lu\n", name, val);
    } else {
      fprintf(stderr, "Invalid value: %lu\n", val);
    }
    ret_val = -1;
  }

  if(!ret_val) {
    *ret = (unsigned char)val;
    return 0;
  }

  fprintf(stderr, "Expected value in the range 0-255\n");
  return -1;
}


static int parse_control(control *out, const char *ctl_str) {
  if(!out || !ctl_str) {
    return -1;
  }

  control_type type;
  if(ctl_str[0] == 'j' || ctl_str[0] == 'J') {
    type = TYPE_JOYSTICK;
  } else if(ctl_str[0] == 'B' || ctl_str[0] == 'b') {
    type = TYPE_BUTTON;
  } else if(ctl_str[0] == 'H' || ctl_str[0] == 'h') {
    type = TYPE_HIGH_POWER_LED;
  } else {
    fprintf(stderr, "Invalid control index: %s\n", ctl_str);
    fprintf(stderr, "Expected value of the format: J#, B#, H#\n");
    fprintf(stderr, "  For example get the status of BUTT2 with 'B2'\n");
    return -1;
  }

  unsigned char index;
  if(parse_byte(&index, ctl_str + 1, "index") < 0) {
    return -1;
  }

  if(type == TYPE_JOYSTICK && (index < 1 || index > 4)) {
    fprintf(stderr, "Invalid joystick index: %d\n", index);
    fprintf(stderr, "Expecting value in the range 1-4\n", index);
    return -1;
  } else if(type == TYPE_BUTTON && (index < 1 || index > 26)) {
    fprintf(stderr, "Invalid button index: %d\n", index);
    fprintf(stderr, "Expecting value in the range 1-26\n", index);
    return -1;
  } else if(type == TYPE_HIGH_POWER_LED && (index < 1 || index > 2)) {
    fprintf(stderr, "Invalid high power LED index: %d\n", index);
    fprintf(stderr, "Expecting value in the range 1-2\n", index);
    return -1;
  }

  out->type = type;
  out->index = index;
  return 0;
}

static int set_led_channel(howler_device *device, int cmd_idx, const char **argv, int argc) {
  if((argc - cmd_idx) != 4) {
    print_usage();
    return -1;
  }

  control c;
  if(parse_control(&c, argv[cmd_idx + 1]) < 0) {
    return -1;
  }

  howler_led_channel_name channel;
  if(strncmp(argv[cmd_idx + 2], "red", 3) == 0) {
    channel = HOWLER_LED_CHANNEL_RED;
  } else if(strncmp(argv[cmd_idx + 2], "green", 5) == 0) {
    channel = HOWLER_LED_CHANNEL_GREEN;
  } else if(strncmp(argv[cmd_idx + 2], "blue", 4) == 0) {
    channel = HOWLER_LED_CHANNEL_BLUE;
  } else {
    print_usage();
    return -1;
  }

  unsigned char value;
  if(parse_byte(&value, argv[cmd_idx + 3], "LED value") < 0) {
    return -1;
  }

  switch(c.type) {
    case TYPE_JOYSTICK:
    {
      if(howler_set_joystick_led_channel(device, c.index, channel, value) < 0) {
        fprintf(stderr, "INTERNAL ERROR: Unable to set LED\n");
        return -1;
      }
    }
    break;

    case TYPE_BUTTON:
    {
      if(howler_set_button_led_channel(device, c.index, channel, value) < 0) {
        fprintf(stderr, "INTERNAL ERROR: Unable to set LED\n");
        return -1;
      }
    }
    break;

    case TYPE_HIGH_POWER_LED:
    {
      if(howler_set_high_power_led_channel(device, c.index, channel, value) < 0) {
        fprintf(stderr, "INTERNAL ERROR: Unable to set LED\n");
        return -1;
      }
    }
    break;
  }

  return 0;
}

static int set_led(howler_device *device, int cmd_idx, const char **argv, int argc) {
  if((argc - cmd_idx) != 5) {
    print_usage();
    return -1;
  }

  control c;
  if(parse_control(&c, argv[cmd_idx + 1]) < 0) {
    return -1;
  }

  howler_led led;
  if((parse_byte(&(led.red), argv[cmd_idx + 2], "Red LED") < 0) ||
     (parse_byte(&(led.green), argv[cmd_idx + 3], "Green LED") < 0) ||
     (parse_byte(&(led.blue), argv[cmd_idx + 4], "Blue LED") < 0)) {
    return -1;
  }

  switch(c.type) {
    case TYPE_JOYSTICK:
    {
      if(howler_set_joystick_led(device, c.index, led) < 0) {
        fprintf(stderr, "INTERNAL ERROR: Unable to set LED\n");
        return -1;
      }
    }
    break;

    case TYPE_BUTTON:
    {
      if(howler_set_button_led(device, c.index, led) < 0) {
        fprintf(stderr, "INTERNAL ERROR: Unable to set LED\n");
        return -1;
      }
    }
    break;

    case TYPE_HIGH_POWER_LED:
    {
      if(howler_set_high_power_led(device, c.index, led) < 0) {
        fprintf(stderr, "INTERNAL ERROR: Unable to set LED\n");
        return -1;
      }
    }
    break;
  }

  return 0;
}

static int print_joystick_led_status(howler_device *device, unsigned char joystick) {
  howler_led led;
  if(howler_get_joystick_led(&led, device, joystick) < 0) {
    return -1;
  }
  
  fprintf(stdout, "Joystick %d LED status: (%d, %d, %d)\n",
          joystick, led.red, led.green, led.blue);
  return 0;
}

static int print_button_led_status(howler_device *device, unsigned char button) {
  howler_led led;
  if(howler_get_button_led(&led, device, button) < 0) {
    return -1;
  }
  
  fprintf(stdout, "Button %d LED status: (%d, %d, %d)\n",
          button, led.red, led.green, led.blue);
  return 0;
}

static int print_high_power_led_status(howler_device *device, unsigned char index) {
  howler_led led;
  if(howler_get_high_power_led(&led, device, index) < 0) {
    return -1;
  }
  
  fprintf(stdout, "High power %d LED status: (%d, %d, %d)\n",
          index, led.red, led.green, led.blue);
  return 0;
}

static int get_led_status(howler_device *device, int cmd_idx, const char **argv, int argc) {
  int button_idx = -1;
  if((argc - cmd_idx) == 2) {

    control c;
    if(parse_control(&c, argv[cmd_idx + 1]) < 0) {
      return -1;
    }

    switch(c.type) {
      default: return -1;
      case TYPE_JOYSTICK: return print_joystick_led_status(device, c.index);
      case TYPE_BUTTON: return print_button_led_status(device, c.index);
      case TYPE_HIGH_POWER_LED: return print_high_power_led_status(device, c.index);
    }

  } else if((argc - cmd_idx) > 2) {
    print_usage();
    return -1;
  }

  if(button_idx < 0) {
    int i = 1;
    int err = 0;
    for(; i <= HOWLER_NUM_JOYSTICKS; i++) {
      err = err || print_joystick_led_status(device, i);
    }
    for(i = 1; i <= HOWLER_NUM_BUTTONS; i++) {
      err = err || print_button_led_status(device, i);
    }
    for(i = 1; i <= HOWLER_NUM_HIGH_POWER_LEDS; i++) {
      err = err || print_high_power_led_status(device, i);
    }
    return err;
  }

  return print_button_led_status(device, button_idx);
}

static int parse_input(howler_input *out, const char *str) {
  if(!out || !str) {
    print_usage();
    return -1;
  }

  if(str[0] == 'J') {
    howler_input joystick = eHowlerInput_Joystick1Up;
    switch(str[1]) {
      case '1': joystick = eHowlerInput_Joystick1Up; break;
      case '2': joystick = eHowlerInput_Joystick2Up; break;
      case '3': joystick = eHowlerInput_Joystick3Up; break;
      case '4': joystick = eHowlerInput_Joystick4Up; break;
      default:
        print_usage();
        return -1;
    }

    switch(str[2]) {
      case 'U': joystick += 0; break;
      case 'D': joystick += 1; break;
      case 'L': joystick += 2; break;
      case 'R': joystick += 3; break;
      default:
        print_usage();
        return -1;
    }

    *out = joystick;
    return 0;
  } else if(str[0] == 'B') {
    int buttonNumber = 0;
    if(sscanf(str + 1, "%d", &buttonNumber) != 1) {
      print_usage();
      return -1;
    }

    if(buttonNumber < 1 || buttonNumber > 26) {
      print_usage();
      return -1;
    }

    *out = eHowlerInput_Button1 + buttonNumber - 1;
    return 0;
  }

  print_usage();
  return -1;
}

static struct {
  howler_key_scan_code code;
  const char *code_str;
} kKeyCodeToStringMap[NUM_HOWLER_KEY_SCAN_CODES] = {
  { eHowlerKeyScanCode_A, "A" },
  { eHowlerKeyScanCode_B, "B" },
  { eHowlerKeyScanCode_C, "C" },
  { eHowlerKeyScanCode_D, "D" },
  { eHowlerKeyScanCode_E, "E" },
  { eHowlerKeyScanCode_F, "F" },
  { eHowlerKeyScanCode_G, "G" },
  { eHowlerKeyScanCode_H, "H" },
  { eHowlerKeyScanCode_I, "I" },
  { eHowlerKeyScanCode_J, "J" },
  { eHowlerKeyScanCode_K, "K" },
  { eHowlerKeyScanCode_L, "L" },
  { eHowlerKeyScanCode_M, "M" },
  { eHowlerKeyScanCode_N, "N" },
  { eHowlerKeyScanCode_O, "O" },
  { eHowlerKeyScanCode_P, "P" },
  { eHowlerKeyScanCode_Q, "Q" },
  { eHowlerKeyScanCode_R, "R" },
  { eHowlerKeyScanCode_S, "S" },
  { eHowlerKeyScanCode_T, "T" },
  { eHowlerKeyScanCode_U, "U" },
  { eHowlerKeyScanCode_V, "V" },
  { eHowlerKeyScanCode_W, "W" },
  { eHowlerKeyScanCode_X, "X" },
  { eHowlerKeyScanCode_Y, "Y" },
  { eHowlerKeyScanCode_Z, "Z" },
  { eHowlerKeyScanCode_1, "1" },
  { eHowlerKeyScanCode_2, "2" },
  { eHowlerKeyScanCode_3, "3" },
  { eHowlerKeyScanCode_4, "4" },
  { eHowlerKeyScanCode_5, "5" },
  { eHowlerKeyScanCode_6, "6" },
  { eHowlerKeyScanCode_7, "7" },
  { eHowlerKeyScanCode_8, "8" },
  { eHowlerKeyScanCode_9, "9" },
  { eHowlerKeyScanCode_0, "0" },
  { eHowlerKeyScanCode_ENTER, "ENTER" },
  { eHowlerKeyScanCode_ESCAPE, "ESCAPE" },
  { eHowlerKeyScanCode_BACKSPACE, "BACKSPACE" },
  { eHowlerKeyScanCode_TAB, "TAB" },
  { eHowlerKeyScanCode_SPACEBAR, "SPACEBAR" },
  { eHowlerKeyScanCode_UNDERSCORE, "UNDERSCORE" },
  { eHowlerKeyScanCode_PLUS, "PLUS" },
  { eHowlerKeyScanCode_OPEN_BRACKET, "OPEN_BRACKET" },
  { eHowlerKeyScanCode_CLOSE_BRACKET, "CLOSE_BRACKET" },
  { eHowlerKeyScanCode_BACKSLASH, "BACKSLASH" },
  { eHowlerKeyScanCode_ASH, "ASH" },
  { eHowlerKeyScanCode_COLON, "COLON" },
  { eHowlerKeyScanCode_QUOTE, "QUOTE" },
  { eHowlerKeyScanCode_TILDE, "TILDE" },
  { eHowlerKeyScanCode_COMMA, "COMMA" },
  { eHowlerKeyScanCode_DOT, "DOT" },
  { eHowlerKeyScanCode_SLASH, "SLASH" },
  { eHowlerKeyScanCode_CAPS_LOCK, "CAPS_LOCK" },
  { eHowlerKeyScanCode_F1, "F1" },
  { eHowlerKeyScanCode_F2, "F2" },
  { eHowlerKeyScanCode_F3, "F3" },
  { eHowlerKeyScanCode_F4, "F4" },
  { eHowlerKeyScanCode_F5, "F5" },
  { eHowlerKeyScanCode_F6, "F6" },
  { eHowlerKeyScanCode_F7, "F7" },
  { eHowlerKeyScanCode_F8, "F8" },
  { eHowlerKeyScanCode_F9, "F9" },
  { eHowlerKeyScanCode_F10, "F10" },
  { eHowlerKeyScanCode_F11, "F11" },
  { eHowlerKeyScanCode_F12, "F12" },
  { eHowlerKeyScanCode_PRINTSCREEN, "PRINTSCREEN" },
  { eHowlerKeyScanCode_SCROLL_LOCK, "SCROLL_LOCK" },
  { eHowlerKeyScanCode_PAUSE, "PAUSE" },
  { eHowlerKeyScanCode_INSERT, "INSERT" },
  { eHowlerKeyScanCode_HOME, "HOME" },
  { eHowlerKeyScanCode_PAGEUP, "PAGEUP" },
  { eHowlerKeyScanCode_DELETE, "DELETE" },
  { eHowlerKeyScanCode_END, "END" },
  { eHowlerKeyScanCode_PAGEDOWN, "PAGEDOWN" },
  { eHowlerKeyScanCode_RIGHT, "RIGHT" },
  { eHowlerKeyScanCode_LEFT, "LEFT" },
  { eHowlerKeyScanCode_DOWN, "DOWN" },
  { eHowlerKeyScanCode_UP, "UP" },
  { eHowlerKeyScanCode_KEYPAD_NUM_LOCK, "KEYPAD_NUM_LOCK" },
  { eHowlerKeyScanCode_KEYPAD_DIVIDE, "KEYPAD_DIVIDE" },
  { eHowlerKeyScanCode_KEYPAD_AT, "KEYPAD_AT" },
  { eHowlerKeyScanCode_KEYPAD_MULTIPLY, "KEYPAD_MULTIPLY" },
  { eHowlerKeyScanCode_KEYPAD_MINUS, "KEYPAD_MINUS" },
  { eHowlerKeyScanCode_KEYPAD_PLUS, "KEYPAD_PLUS" },
  { eHowlerKeyScanCode_KEYPAD_ENTER, "KEYPAD_ENTER" },
  { eHowlerKeyScanCode_KEYPAD_1, "KEYPAD_1" },
  { eHowlerKeyScanCode_KEYPAD_2, "KEYPAD_2" },
  { eHowlerKeyScanCode_KEYPAD_3, "KEYPAD_3" },
  { eHowlerKeyScanCode_KEYPAD_4, "KEYPAD_4" },
  { eHowlerKeyScanCode_KEYPAD_5, "KEYPAD_5" },
  { eHowlerKeyScanCode_KEYPAD_6, "KEYPAD_6" },
  { eHowlerKeyScanCode_KEYPAD_7, "KEYPAD_7" },
  { eHowlerKeyScanCode_KEYPAD_8, "KEYPAD_8" },
  { eHowlerKeyScanCode_KEYPAD_9, "KEYPAD_9" },
  { eHowlerKeyScanCode_KEYPAD_0, "KEYPAD_0" }
};

static int parse_key(howler_key_scan_code *out, const char *str) {
  if(!out || !str) {
    return -1;
  }

  int i = 0;
  for (; i < NUM_HOWLER_KEY_SCAN_CODES; ++i) {
    const char *code_str = kKeyCodeToStringMap[i].code_str;
    if(strncmp(code_str, str, strlen(code_str)) == 0) {
      *out = kKeyCodeToStringMap[i].code;
      return 0;
    }
  }
  print_usage();
  return -1;
}

static int list_supported_keys(howler_device *device, int cmd_idx, const char **argv, int argc) {
  int i = 0;
  for (; i < NUM_HOWLER_KEY_SCAN_CODES; ++i) {
    printf("  %s\n", kKeyCodeToStringMap[i].code_str);
  }
  return 0;
}

static int set_key(howler_device *device, int cmd_idx, const char **argv, int argc) {
  if((argc - cmd_idx) < 3) {
    print_usage();
    return -1;
  }

  howler_input ipt;
  if(parse_input(&ipt, argv[cmd_idx + 1]) < 0) {
    return -1;
  }

  howler_key_scan_code code;
  if(parse_key(&code, argv[cmd_idx + 2]) < 0) {
    return -1;
  }

  if(howler_set_input_keyboard(device, ipt, code,
                               eHowlerKeyModifier_None) < 0) {
    fprintf(stderr, "INTERNAL ERROR: Unable to set keyboard mapping\n");
    return -1;
  }

  return 0;
}

int main(int argc, const char **argv) {
  int exitCode = 0;

  // Make sure that we have the requisite number of arguments....
  if(argc < 2) {
    print_usage();
    exit(1);
  }

  int device_idx_specified = 0;
  int device_idx = parse_device(argv[1]);
  if(device_idx == -2) {
    device_idx = 0;
  } else {
    device_idx_specified = 1;
    if(argc < 3) {
      print_usage();
      exit(1);
    }
  }

  if(device_idx < 0) {
    fprintf(stderr, "Invalid device index\n");
    exit(1);
  }

  howler_context *ctx;
  if(howler_init(&ctx) < 0) {
    fprintf(stderr, "Howler initialization failed.\n");
    exitCode = 1;
    goto done;
  }

  size_t nDevices = howler_get_num_connected(ctx);
  if(!nDevices) {
    fprintf(stderr, "No Howler devices found\n");
    exitCode = 1;
    goto done;
  }

  if(device_idx >= nDevices) {
    fprintf(stderr, "Invalid device number. Only %d device%s available.\n", nDevices, (nDevices > 1)? "s" : "");
    exitCode = 1;
    goto done;    
  }

  howler_device *device = howler_get_device(ctx, device_idx);
  if(!device) {
    fprintf(stderr, "INTERNAL ERROR: Howler devices found but device is invalid?\n");
    exitCode = 1;
    goto done;
  }

  int cmd_idx = (device_idx_specified)? 2 : 1;
  assert(cmd_idx < argc);

  const char *cmd = argv[cmd_idx];

  command_function cmdFn = NULL;
  if(strncmp(cmd, "help", 4) == 0) {
    print_usage();

  } else if(strncmp(cmd, "get-firmware", 12) == 0) {
    char versionBuf[256];
    get_firmware(device, versionBuf, 256);
    printf("Firmware version: %s\n", versionBuf);
    goto done;
  } else if(strncmp(cmd, "list-supported-keys", 19) == 0) {
    cmdFn = list_supported_keys;
  } else if(strncmp(cmd, "get-led", 7) == 0) {
    cmdFn = &get_led_status;
  } else if(strncmp(cmd, "set-led-channel", 15) == 0) {
    cmdFn = &set_led_channel;
  } else if(strncmp(cmd, "set-led", 7) == 0) {
    cmdFn = &set_led;
  } else if(strncmp(cmd, "set-key", 7) == 0) {
    cmdFn = &set_key;
  } else {
    print_usage();
    exitCode = 1;
    goto done;
  }

  assert(cmdFn);
  if((*cmdFn)(device, cmd_idx, argv, argc) < 0) {
    exitCode = 1;
    goto done;
  }

 done:
  howler_destroy(ctx);
  return exitCode;
}
