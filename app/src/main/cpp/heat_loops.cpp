/* Copyright (C) 2010 Zhunping Zhang
 *
 * Code for 6.884 Lab 3
 *
 * Modified by Jim Sukha, February 2010
 * Modified and adapted for Android by Tao B. Schardl, December 2024
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "common.h"
#include "sim.h"

void rect_loops_serial(const SimState *Q,
                       int t0, int t1,
                       int x0, int x1,
                       int my_y0, int my_y1) {
  for (int t = t0; t < t1; t++) {
    for (int x = x0; x < x1; x++) {
      int y;
      for (y = my_y0; y + 7 < my_y1; y += 8) {
        Q->kernel(t, x, y);
        Q->kernel(t, x, y + 1);
        Q->kernel(t, x, y + 2);
        Q->kernel(t, x, y + 3);
        Q->kernel(t, x, y + 4);
        Q->kernel(t, x, y + 5);
        Q->kernel(t, x, y + 6);
        Q->kernel(t, x, y + 7);
      }
      for (; y < my_y1; y++) {
        Q->kernel(t, x, y);
      }
    }
  }
}

void rect_loops_parallel(const SimState *Q,
                         int t0, int t1,
                         int x0, int x1,
                         int my_y0, int my_y1) {

  /* This is the first parallel version of heat equation,
     which utilize the cilk_for
   */
  int t;
  assert(Q->Xsep > 0);
  assert(Q->Ysep > 0);

  for (t = t0; t < t1; t++) {
    cilk_for (int x = x0; x < x1; x++) {
      cilk_for(int y = my_y0; y < my_y1; y++) {
        Q->kernel(t, x, y);
      }
    }
  }
}
