/* Copyright (C) 2006 Matteo Frigo
 *
 * Modified and used with permission for 6.884 Lab 3
 * Modified by Zhunping Zhang and Jim Sukha, Feburary 2010
 * Ported to Android by Tao B. Schardl, December 2024
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

#define ltThresh 32
// Serial recursive cache-oblivious code for stencil computation.
static const int ds = 1;
void walk2(const SimState* Q,
           int t0, int t1,
           int x0, int dx0, int x1, int dx1,
           int my_y0, int dmy_y0, int my_y1, int dmy_y1) {
  int lt = t1 - t0;
  if (lt == 1) {
    Q->kernel_single_timestep(t0, x0, x1, my_y0, my_y1);
  } else if (lt > 1) {
    if (2 * (x1 - x0) + (dx1 - dx0) * lt >= 4 * ds * lt) {
      int xm = (2 * (x0 + x1) + (2 * ds + dx0 + dx1) * lt) / 4;
      walk2(Q, t0, t1, x0, dx0, xm, -ds, my_y0, dmy_y0, my_y1, dmy_y1);
      walk2(Q, t0, t1, xm, -ds, x1, dx1, my_y0, dmy_y0, my_y1, dmy_y1);

      // The old version
      // {
      //      walk2(Q, t0, t1, x0, dx0, xm, -ds, my_y0, dmy_y0, my_y1, dmy_y1);
      //      walk2(Q, t0, t1, xm, -ds, x1, dx1, my_y0, dmy_y0, my_y1, dmy_y1);
      // }

      /* int halft = lt/2; */

      /* // One corner? */
      /* walk2(Q, t0, t0+halft, x0, dx0, xm, -ds, my_y0, dmy_y0, my_y1, dmy_y1); */

      /* // Middle two pieces can run in parallel. */
      /* walk2(Q, */
      /*       t0+halft, t1, */
      /*       x0 + dx0 * halft, dx0, */
      /*       xm + (-ds) * halft, -ds, */
      /*       my_y0 + dmy_y0 * halft, dmy_y0, */
      /*       my_y1 +  dmy_y1  * halft, dmy_y1); */

      /* walk2(Q, t0, t0+halft, xm, -ds, x1, dx1, my_y0, dmy_y0, my_y1, dmy_y1); */

      /* // Upper corner? */
      /* walk2(Q, t0+halft, t1, */
      /*       xm + (-ds) * halft, -ds, x1 + dx1 * halft, dx1, */
      /*       my_y0 +  dmy_y0  * halft, dmy_y0, my_y1 + dmy_y1 * halft, dmy_y1); */

    } else if (2 * (my_y1 - my_y0) + (dmy_y1 - dmy_y0) * lt >= 4 * ds * lt) {
      int ym = (2 * (my_y0 + my_y1) + (2 * ds + dmy_y0 + dmy_y1) * lt) / 4;

      walk2(Q, t0, t1, x0, dx0, x1, dx1, my_y0, dmy_y0, ym, -ds);
      walk2(Q, t0, t1, x0, dx0, x1, dx1, ym, -ds, my_y1, dmy_y1);

      // Old version
      // {
      //    spawn walk2(Q, t0, t1, x0, dx0, x1, dx1, my_y0, dmy_y0, ym, -ds);
      //    sync;
      //    spawn walk2(Q, t0, t1, x0, dx0, x1, dx1, ym, -ds, my_y1, dmy_y1);
      //    sync;
      // }

      /* int halft = lt/2; */

      /* walk2(Q, t0, t0+halft, */
      /*       x0, dx0, x1, dx1, */
      /*       my_y0, dmy_y0, ym, -ds); */

      /* // Two middle values. */
      /* walk2(Q, t0+halft, t1, */
      /*       x0 + dx0 * halft, dx0, x1 +  dx1  * halft, dx1, */
      /*       my_y0 + dmy_y0 * halft, dmy_y0, ym + (-ds) * halft, -ds); */
      /* walk2(Q, t0, t0+halft, */
      /*       x0, dx0, x1, dx1, */
      /*       ym, -ds, my_y1, dmy_y1); */

      /* walk2(Q, t0+halft, t1, */
      /*       x0 +  dx0  * halft, dx0, x1 + dx1 * halft, dx1, */
      /*       ym + (-ds) * halft, -ds, my_y1 + dmy_y1 * halft, dmy_y1); */

    } else {
      if (lt > ltThresh) {
        int halflt = lt / 2;
        walk2(Q, t0, t0 + halflt, x0, dx0, x1, dx1, my_y0, dmy_y0, my_y1, dmy_y1);
        walk2(Q, t0 + halflt, t1,
              x0 + dx0 * halflt, dx0, x1 + dx1 * halflt, dx1,
              my_y0 + dmy_y0 * halflt, dmy_y0, my_y1 + dmy_y1 * halflt, dmy_y1);
      } else {
        for (int t = 0; t < lt; ++t) {
          // loop unrolled
          for (int x = x0; x < x1; x++) {
            int y;
            for (y = my_y0; y + 7 < my_y1; y += 8) {
              Q->kernel(t0 + t, x, y);
              Q->kernel(t0 + t, x, y + 1);
              Q->kernel(t0 + t, x, y + 2);
              Q->kernel(t0 + t, x, y + 3);
              Q->kernel(t0 + t, x, y + 4);
              Q->kernel(t0 + t, x, y + 5);
              Q->kernel(t0 + t, x, y + 6);
              Q->kernel(t0 + t, x, y + 7);
            }
            for (; y < my_y1; y++) {
              Q->kernel(t0 + t, x, y);
            }
          }
          x0 += dx0;
          my_y0 += dmy_y0;
          x1 += dx1;
          my_y1 += dmy_y1;
        }
      }
    }
  }
}

void rect_recursive_serial(const SimState* Q,
                           int t0, int t1,
                           int x0, int x1,
                           int my_y0, int my_y1) {
  walk2(Q,
        t0, t1,
        x0, 0,
        x1, 0,
        my_y0, 0,
        my_y1, 0);
}
