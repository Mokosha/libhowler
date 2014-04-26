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

  howler_device *device = howler_get_device(ctx, 0);
  if(!device) {
    exitCode = 1;
    goto done;
  }

  char versionBuf[256];
  if(howler_get_device_version(device, versionBuf, 256, NULL) < 0) {
    exitCode = 1;
    goto done;
  }

  printf("Version string: %s\n", versionBuf);

 done:
  howler_destroy(ctx);
  return exitCode;
}
