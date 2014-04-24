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

typedef struct {
  libusb_device *usb_device;
  libusb_device_handle *usb_handle;
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

/* Initialize the Howler context. This context interfaces with all of the
 * identifiable howler controllers. It first makes sure to initialize libusb
 * in order to send commands. It returns 0 on success, and an error otherwise
 */
int howler_init(howler_context **);
void howler_destroy(howler_context *);

size_t howler_get_num_connected(howler_context *ctx);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // __HOWLER_LIB_H__
