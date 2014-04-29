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

static void print_version() {
  printf("HowlerCtl Version 0.0.1\n");
}

static void print_usage() {
  print_version();
  printf("Usage: howler [DEVICE] COMMAND [OPTIONS]\n");
  printf("\n");
  printf("    DEVICE is a number from 0 to 4 that designates the\n");
  printf("    corresponding Howler device. The default is 0.\n");
  printf("\n");
  printf("    COMMAND is one of the following:\n");
  printf("        help\n");
  printf("        get-firmware\n");
  printf("        get-led [CONTROL]\n");
  printf("        set-led-channel CONTROL (red|green|blue) VALUE\n");
  printf("        set-led CONTROL RED GREEN BLUE\n");
  printf("\n");
  printf("    CONTROL is a string conforming to one of the following:\n");
  printf("        J1 - J4: Joystick 1 to Joystick 4\n");
  printf("        B1 - B26: Button 1 to Button 26\n");
  printf("        H1 - H2: High Power LED 1 or 2\n");
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

  if(strncmp(cmd, "help", 4) == 0) {
    print_usage();

  } else if(strncmp(cmd, "get-firmware", 12) == 0) {
    char versionBuf[256];
    get_firmware(device, versionBuf, 256);
    printf("Firmware version: %s\n", versionBuf);

  } else if(strncmp(cmd, "get-led", 7) == 0) {
    if(get_led_status(device, cmd_idx, argv, argc) < 0) {
      exitCode = 1;
      goto done;
    }

  } else if(strncmp(cmd, "set-led-channel", 15) == 0) {
    if(set_led_channel(device, cmd_idx, argv, argc) < 0) {
      exitCode = 1;
      goto done;
    }

  } else if(strncmp(cmd, "set-led", 7) == 0) {
    if(set_led(device, cmd_idx, argv, argc) < 0) {
      exitCode = 1;
      goto done;
    }

  } else {
    print_usage();
    exitCode = 1;
    goto done;
  }

 done:
  howler_destroy(ctx);
  return exitCode;
}
