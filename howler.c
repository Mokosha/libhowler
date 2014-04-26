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

static const unsigned short HOWLER_VENDOR_ID = 0x3EB;

#define MAX_HOWLER_DEVICE_IDS 4
static const unsigned short HOWLER_DEVICE_ID[MAX_HOWLER_DEVICE_IDS] =
  { 0x6800, 0x6801, 0x6802, 0x6803 };

// Howler commands
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

static int is_howler(libusb_device *device) {
  struct libusb_device_descriptor desc;
  libusb_get_device_descriptor(device, &desc);

  if(desc.idVendor != HOWLER_VENDOR_ID) {
    return 0;
  }

  int has_device_id = 0;
  unsigned int i = 0;
  for(; i < MAX_HOWLER_DEVICE_IDS; i++) {
    if(desc.idProduct == HOWLER_DEVICE_ID[i]) {
      has_device_id = 1;
    }
  }

  if(!has_device_id) {
    return 0;
  }
  return 1;
}

int howler_init(howler_context **ctx_ptr) {
  // Convenience variables
  unsigned int i;

  // First, check if the pointer is valid...
  int error = HOWLER_SUCCESS;
  if(!ctx_ptr) { return HOWLER_ERROR_INVALID_PTR; }

  // Allocate a USB context so that we can talk to the devices...
  libusb_context *usb_ctx;
  int ret = libusb_init(&usb_ctx);
  if(ret) {
    error = HOWLER_ERROR_LIBUSB_CONTEXT_ERROR;
    goto err;
  }

  libusb_set_debug(usb_ctx, 3);

  // Get a list of all available devices.
  libusb_device **device_list;
  ssize_t nDevices = libusb_get_device_list(usb_ctx, &device_list);
  if(nDevices < 0) {
    error = HOWLER_ERROR_LIBUSB_DEVICE_LIST_ERROR;
    goto err_after_libusb_context;
  }

  // Find which ones are howlers...
  size_t nHowlers = 0;
  for(i = 0; i < nDevices; i++) {
    if(is_howler(device_list[i])) {
      nHowlers++;
    }
  }

  // Short circuit if no howlers...
  if(!nHowlers) {
    howler_context *result = malloc(sizeof(howler_context));
    result->usb_ctx = usb_ctx;
    result->nDevices = nHowlers;
    result->devices = NULL;
    *ctx_ptr = result;
  }

  howler_device *howlers = malloc(nHowlers * sizeof(howler_device));
  unsigned int howler_idx = 0;
  for(i = 0; i < nDevices; i++) {
    if(!is_howler(device_list[i]))
      continue;

    libusb_device *d = device_list[i];
    libusb_device_handle *h = NULL;
    int err = libusb_open(d, &h);
    if(err < 0) {
      if(err == LIBUSB_ERROR_ACCESS) {
        fprintf(stderr,
                "WARNING: Unable to open interface to Howler device: "
                "Permission Denied\n");
      }

      // Just skip this device...
      nHowlers--;
      continue;
    }
    
    assert(howler_idx < nHowlers);
    howler_device *howler = &(howlers[howler_idx]);
    howler->usb_device = d;
    howler->usb_handle = h;
    howler_idx++;
  }

  assert(howler_idx == nHowlers);

  // Everything is OK...
  howler_context *result = malloc(sizeof(howler_context));
  result->usb_ctx = usb_ctx;
  result->nDevices = nHowlers;
  result->devices = howlers;
  *ctx_ptr = result;

  // Cleanup
  libusb_free_device_list(device_list, 1);
  return HOWLER_SUCCESS;

  // Errors...
 err_after_libusb_context:
  libusb_exit(usb_ctx);
 err:
  *ctx_ptr = NULL;
  return error;
}

void howler_destroy(howler_context *ctx) {
  if(!ctx) { return; }
  unsigned int i = 0;
  for(; i < ctx->nDevices; i++) {
    libusb_close(ctx->devices[i].usb_handle);
  }
  free(ctx->devices);

  libusb_exit(ctx->usb_ctx);
  free(ctx);
}

size_t howler_get_num_connected(howler_context *ctx) {
  return ctx->nDevices;
}

howler_device *howler_get_device(howler_context *ctx, unsigned int device_index) {
  if(!ctx) { return NULL; }
  if(device_index >= howler_get_num_connected(ctx)) { return NULL; }
  return ctx->devices + device_index;
}

static int howler_sendrcv(howler_device *dev,
                          unsigned char *cmd_buf,
                          unsigned char *output) {
  // Claim the interface. Make sure the kernel driver is not attached 
  // first, however.
  libusb_device_handle *handle = dev->usb_handle;
  int err = libusb_kernel_driver_active(handle, 0);
  if(err < 0) {
    return -1;
  }

  int kernel_driver_attached = 0;
  if(err) {
    kernel_driver_attached = 1;
    err = libusb_detach_kernel_driver(handle, 0);
    if(err < 0) {
      return -1;
    }
  }

  err = libusb_claim_interface(handle, 0);
  if(err < 0) {
    goto detach;
  }

  // Write the command
  int transferred = 0;
  err = libusb_interrupt_transfer(handle, 0x02, cmd_buf, 24, &transferred, 0);
  if(err < 0) {
    goto error;
  }

  // Read the following command
  if(output) {
    unsigned char endpoint = 0x81;
    while(err = libusb_interrupt_transfer(handle, endpoint, output, 24, &transferred, 0)) {
      if(0x81 == endpoint) {
        endpoint = 0x83;
      } else if(0x83 == endpoint) {
        endpoint = 0x86;
      } else {
        break;
      }
    }
  }

 error:
  libusb_release_interface(handle, 0);
 detach:
  if(kernel_driver_attached) {
    libusb_attach_kernel_driver(handle, 0);
  }
  return err;
}

/* Sets an LED bank to the given RGB values */
typedef howler_led_channel howler_led_bank[16];
static int howler_set_led_bank(howler_device *dev, unsigned char bank_idx,
                               const howler_led_bank *bank);


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

int howler_set_led(howler_device *dev, unsigned char index, howler_led led) {
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

int howler_set_led_channel(howler_device *dev, unsigned char index,
                   howler_led_channel_name channel, howler_led_channel val) {
  if(channel >= 85) {
    fprintf(stderr, "ERROR: howler_set_led invalid index: %d\n", index);
    return -1;
  }

  unsigned char cmd_buf[24];
  memset(cmd_buf, 0, sizeof(cmd_buf));

  cmd_buf[0] = CMD_HOWLER_ID;
  cmd_buf[1] = CMD_SET_INDIVIDUAL_LED;
  cmd_buf[2] = 3*index + (unsigned char)channel;
  cmd_buf[3] = val;

  return howler_sendrcv(dev, cmd_buf, NULL);
}

int howler_set_led_bank(howler_device *dev, unsigned char index,
                        const howler_led_bank *bank) {
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
