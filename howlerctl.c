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

#include "howlerlib.h"

static void print_version() {
  printf("HowlerCtl Version 0.0.1\n");
}

static void print_usage() {
  print_version();
  printf("Usage: howler [DEVICE] COMMAND [OPTIONS]\n");
  printf("\n");
  printf("    Where DEVICE is a number from 0 to 4 that designates the\n");
  printf("    corresponding Howler device. The default is 0.\n");
  printf("\n");
  printf("    Where COMMAND is one of the following:\n");
  printf("        help\n");
  printf("        get-firmware\n");
  printf("        set-led LED VALUE\n");
  printf("        set-rgb-led LED RED GREEN BLUE\n");
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
  }

  if(val > 255) {
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

static int set_led(howler_device *device, int cmd_idx, const char **argv, int argc) {
  if((argc - cmd_idx) != 3) {
    print_usage();
    return -1;
  }

  unsigned char index;
  if(parse_byte(&index, argv[cmd_idx + 1], "LED index") < 0) {
    return -1;
  }

  unsigned char value;
  if(parse_byte(&value, argv[cmd_idx + 2], "LED value") < 0) {
    return -1;
  }

  if(howler_set_led(device, index, value) < 0) {
    fprintf(stderr, "INTERNAL ERROR: Unable to set LED\n");
    return -1;
  }
}

static int set_led_rgb(howler_device *device, int cmd_idx, const char **argv, int argc) {
  if((argc - cmd_idx) != 5) {
    print_usage();
    return -1;
  }

  unsigned char index;
  if(parse_byte(&index, argv[cmd_idx + 1], "LED index") < 0) {
    return -1;
  }

  unsigned char red;
  if(parse_byte(&red, argv[cmd_idx + 2], "Red LED") < 0) {
    return -1;
  }

  unsigned char green;
  if(parse_byte(&green, argv[cmd_idx + 3], "Green LED") < 0) {
    return -1;
  }

  unsigned char blue;
  if(parse_byte(&blue, argv[cmd_idx + 4], "Blue LED") < 0) {
    return -1;
  }

  if(howler_set_led_rgb(device, index, red, green, blue) < 0) {
    fprintf(stderr, "INTERNAL ERROR: Unable to set LED\n");
    return -1;
  }
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
  howler_init(&ctx);

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

  } else if(strncmp(cmd, "set-led", 7) == 0) {
    if(set_led(device, cmd_idx, argv, argc) < 0) {
      exitCode = 1;
      goto done;
    }

  } else if(strncmp(cmd, "set-rgb-led", 11) == 0) {
    if(set_led_rgb(device, cmd_idx, argv, argc) < 0) {
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
