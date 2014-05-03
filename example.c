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

#include <stdio.h>
#include <string.h>

#include "howler.h"

int main() {
  int exitCode = 0;

  howler_context *ctx;
  howler_init(&ctx);

  size_t nDevices = howler_get_num_connected(ctx);
  printf("Num connected howlers: %d\n", nDevices);

  if(!nDevices) {
    exitCode = 1;
    goto done;
  }

  howler_device *dev = howler_get_device(ctx, 0);
  if(!dev) {
    exitCode = 1;
    goto done;
  }

  char versionBuf[256];
  if(howler_get_device_version(dev, versionBuf, 256, NULL) < 0) {
    exitCode = 1;
    goto done;
  }

  printf("Version string: %s\n", versionBuf);

  // Claim the interface. Make sure the kernel driver is not attached 
  // first, however.
  libusb_device_handle *handle = (libusb_device_handle *)(dev->usb_handle);

  // Read the following command
  int transferred = 0;
  char output[24];
  memset(output, 0, sizeof(output));

  // int endpoints[] = { 0x81, 0x83, 0x84, 0x85, 0x86 };
  int endpoints[] = { 0x81, 0x83, 0x84, 0x85, 0x86 };
  int interface = 0;
  const int nInterfaces = sizeof(endpoints) / sizeof(endpoints[0]);
  while(1) {
    fprintf(stdout, "Claiming interface %d...\n", interface);
    exitCode = libusb_kernel_driver_active(handle, interface);
    if(exitCode < 0) {
      exitCode = 1;
      goto done;
    }

    int kernel_driver_attached = 0;
    if(exitCode) {
      kernel_driver_attached = 1;
      exitCode = libusb_detach_kernel_driver(handle, interface);
      if(exitCode < 0) {
        goto done;
      }
    }

    exitCode = libusb_claim_interface(handle, interface);
    if(exitCode < 0) {
      goto unclaim;
    }

    int endpoint = endpoints[interface];
    fprintf(stdout, "Listening on endpoint 0x%x...\n", endpoint);
    exitCode = libusb_interrupt_transfer(handle, endpoint, output, 24, &transferred, 2000);

  unclaim:
    libusb_release_interface(handle, interface);
    if(kernel_driver_attached) {
      libusb_attach_kernel_driver(handle, 0);
    }

    if(exitCode == LIBUSB_SUCCESS || exitCode == LIBUSB_ERROR_TIMEOUT) {
      if(transferred > 0) {
        fprintf(stdout, "Received input on endpoint 0x%x:", endpoint);
        int i = 0;
        for(; i < 24; i++) {
          fprintf(stdout, " 0x%x", output[i]);
        }
        fprintf(stdout, "\n");
      } else {
        interface = (interface + 1) % nInterfaces;
      }
    }

    if(exitCode != LIBUSB_ERROR_TIMEOUT && exitCode < 0) {
      fprintf(stderr, "libusb encountered an error: %d\n", exitCode);
      break;
    }
  }

 detach:
 done:
  howler_destroy(ctx);
  return exitCode;
}
