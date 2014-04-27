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

unsigned char howler_button_to_bank[HOWLER_NUM_BUTTONS][3][2] = {
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

unsigned char howler_joystick_to_bank[HOWLER_NUM_JOYSTICKS][3][2] = {
  { { 0, 0 }, { 2, 0 }, { 4, 0 } },
  { { 0, 15 }, { 2, 15 }, { 4, 15 } },
  { { 1, 6 }, { 3, 6 }, { 5, 6 } },
  { { 1, 9 }, { 3, 9 }, { 5, 9 } },
};

unsigned char howler_hp_led_to_bank[HOWLER_NUM_HIGH_POWER_LEDS][3][2]
= {
  { { 1, 7 }, { 3, 7 }, { 5, 7 } },
  { { 1, 8 }, { 3, 8 }, { 5, 8 } },
};
