/* Copyright (C) 2006 Matteo Frigo
 *
 * Modified and used with permission for 6.884 Lab 3
 * Modified by Jim Sukha, Yuan Tang, Steven Bartel, Dina Kachintseva May 2010
 *

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#include <cilk/cilk.h>
#include <cstdio>
#include <cstdlib>
#include <string>
//#include <iostream>
//#include <fstream>

#include "common.h"
#include "sim.h"

void debug(const char* name, int t0, int t1, int x0, int dx0, int x1, int dx1,
           int y0, int dy0, int y1, int dy1) {
#if S_DEBUG
  fprintf(stderr,
          "%s : t0 = %d, t1 = %d, x0 = %d, dx0 = %d, x1 = %d, dx1 = %d, y0 = %d, dy0 = %d, y1 = %d, dy1 = %d\n",
          name, t0, t1, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
#endif
}

// Serial recursive cache-oblivious code for stencil computation.
static const int ds = 1;
static const int coarsen = 5;

static inline void walk_dp_t(const SimState* Q,
                             int t0, int t1,
                             int x0, int dx0, int x1, int dx1,
                             int y0, int dy0, int y1, int dy1) {
  /* This is steve's original dual-partition version
   */
  int lt = t1 - t0;
  if (lt <= coarsen && lt > 0) {
    for (int i = t0; i < t1; i++) {
      Q->kernel_single_timestep(i, x0, x1, y0, y1);
      x0 += dx0;
      x1 += dx1;
      y0 += dy0;
      y1 += dy1;
    }
  } else if (lt > coarsen) {
    if (dx0 >= dx1 && x1 - x0 > 4 * ds * lt) { // bottom >= top && bottom sufficiently large
      int mid = (x0 + x1) / 2;
      cilk_scope {
          cilk_spawn walk_dp_t(Q, t0, t1, x0, dx0, mid, -ds, y0, dy0, y1, dy1); // bottom left triangle
          cilk_spawn walk_dp_t(Q, t0, t1, mid, ds, x1, dx1, y0, dy0, y1, dy1); // bottom right triangle
      }
      walk_dp_t(Q, t0, t1, mid, -ds, mid, ds, y0, dy0, y1, dy1); // top mid triangle
    } else if (dx1 < dx0
               && x1 + dx1 - x0 - dx0 > 4 * ds * lt) { // top > bottom && top sufficiently large
      int mid = (x0 + dx0 * lt + x1 + dx1 * lt) / 2;

      int bLeft = mid - ds * lt;
      int bRight = mid + ds * lt;

      walk_dp_t(Q, t0, t1, bLeft, ds, bRight, -ds, y0, dy0, y1, dy1); // bottom mid triangle
      cilk_spawn walk_dp_t(Q, t0, t1, x0, dx0, bLeft, ds, y0, dy0, y1, dy1); // top left triangle
      /* ds ? or -ds? */
      cilk_spawn walk_dp_t(Q, t0, t1, bRight, ds, x1, dx1, y0, dy0, y1, dy1); // top right triangle
      cilk_sync;
    } else if (dy0 >= dy1 && y1 - y0 > 4 * ds * lt) {
      int mid = (y0 + y1) / 2;
      cilk_spawn walk_dp_t(Q, t0, t1, x0, dx0, x1, dx1, y0, dy0, mid, -ds); // bottom left triangle
      cilk_spawn walk_dp_t(Q, t0, t1, x0, dx0, x1, dx1, mid, ds, y1, dy1); // bottom right triangle
      cilk_sync;
      walk_dp_t(Q, t0, t1, x0, dx0, x1, dx1, mid, -ds, mid, ds); // top mid triangle
    } else if (dy1 < dy0
               && y1 + dy1 - y0 - dy0 > 4 * ds * lt) { // top > bottom && top sufficiently large
      int mid = (y0 + dy0 * lt + y1 + dy1 * lt) / 2;

      int bLeft = mid - ds * lt;
      int bRight = mid + ds * lt;

      walk_dp_t(Q, t0, t1, x0, dx0, x1, dx1, bLeft, ds, bRight, -ds); // bottom mid triangle
      cilk_spawn walk_dp_t(Q, t0, t1, x0, dx0, x1, dx1, y0, dy0, bLeft, ds); // top left triangle
      cilk_spawn walk_dp_t(Q, t0, t1, x0, dx0, x1, dx1, bRight, ds, y1, dy1); // top right triangle
      cilk_sync;
    } else {
      int halflt = lt / 2;
      walk_dp_t(Q, t0, t0 + halflt, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
      walk_dp_t(Q, t0 + halflt, t1,
                x0 + dx0 * halflt, dx0, x1 + dx1 * halflt, dx1,
                y0 + dy0 * halflt, dy0, y1 + dy1 * halflt, dy1);
    }


    /*if (2 * (x1 - x0) + (dx1 - dx0) * lt >= 4 * ds * lt) {
      int xm = (2 * (x0 + x1) + (2 * ds + dx0 + dx1) * lt) / 4;
      walk_dp_t(Q, t0, t1, x0, dx0, xm, -ds, y0, dy0, y1, dy1);
      walk_dp_t(Q, t0, t1, xm, -ds, x1, dx1, y0, dy0, y1, dy1);
    } else if (2 * (y1 - y0) + (dy1 - dy0) * lt >= 4 * ds * lt) {
      int ym = (2 * (y0 + y1) + (2 * ds + dy0 + dy1) * lt) / 4;
      walk_dp_t(Q, t0, t1, x0, dx0, x1, dx1, y0, dy0, ym, -ds);
      walk_dp_t(Q, t0, t1, x0, dx0, x1, dx1, ym, -ds, y1, dy1);
    } else {
      int halflt = lt / 2;
      walk_dp_t(Q, t0, t0 + halflt, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
      walk_dp_t(Q, t0 + halflt, t1,
          x0 + dx0 * halflt, dx0, x1 + dx1 * halflt, dx1,
          y0 + dy0 * halflt, dy0, y1 + dy1 * halflt, dy1);
    }*/
  }
}

#define X_STOP 64 // 100
#define Y_STOP 64 // 100
#define SLOPE_X 1
#define SLOPE_Y 1
#define DT_STOP 5

static inline void walk_dp_xyt(const SimState* Q,
                               int t0, int t1,
                               int x0, int dx0, int x1, int dx1,
                               int y0, int dy0, int y1, int dy1) {
  /* This is steve's original dual-partition version
   */
  int lt = t1 - t0;
  int cur_bl_x = x1 - x0;
  int cur_bl_y = y1 - y0;
  int cur_ul_x = x1 + dx1 - x0 - dx0;
  int cur_ul_y = y1 + dy1 - y0 - dy0;
  int x_cut_thres = 4 * SLOPE_X * lt;
  int y_cut_thres = 4 * SLOPE_Y * lt;
  /* By well-shaped, we mean bottom >= top, otherwise, it's !wellShaped
   */
  bool xwellShaped = dx0 >= 0 && dx1 <= 0;
  bool ywellShaped = dy0 >= 0 && dy1 <= 0;
  if (((xwellShaped && cur_bl_x <= X_STOP) ||
       (!xwellShaped && cur_ul_x <= X_STOP)) &&
      ((ywellShaped && cur_bl_y <= Y_STOP) ||
       (!ywellShaped && cur_ul_y <= X_STOP)) &&
      lt <= DT_STOP) {
    Q->base_case_kernel(t0, t1, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
    return;
  } else  {
    if (xwellShaped && cur_bl_x > x_cut_thres
        && cur_bl_x > X_STOP) { // bottom >= top && bottom sufficiently large
      int mid = (x0 + x1) / 2;
      cilk_scope {
          cilk_spawn walk_dp_xyt(Q, t0, t1, x0, dx0, mid, -SLOPE_X, y0, dy0, y1, dy1); // bottom left triangle
          cilk_spawn walk_dp_xyt(Q, t0, t1, mid, SLOPE_X, x1, dx1, y0, dy0, y1, dy1); // bottom right triangle
      }
      walk_dp_xyt(Q, t0, t1, mid, -SLOPE_X, mid, SLOPE_X, y0, dy0, y1, dy1); // top mid triangle
      return;
    } else if (!xwellShaped && cur_ul_x > x_cut_thres
               && cur_ul_x > X_STOP) { // top > bottom && top sufficiently large
      int mid = (x0 + dx0 * lt + x1 + dx1 * lt) / 2;

      int bLeft = mid - SLOPE_X * lt;
      int bRight = mid + SLOPE_X * lt;

      walk_dp_xyt(Q, t0, t1, bLeft, SLOPE_X, bRight, -SLOPE_X, y0, dy0, y1, dy1); // bottom mid triangle
      cilk_scope {
          cilk_spawn walk_dp_xyt(Q, t0, t1, x0, dx0, bLeft, SLOPE_X, y0, dy0, y1, dy1); // top left triangle
          /* ds ? or -ds? */
          cilk_spawn walk_dp_xyt(Q, t0, t1, bRight, -SLOPE_X, x1, dx1, y0, dy0, y1,
          dy1); // top right triangle
      }
      return;
    } else if (ywellShaped && cur_bl_y > y_cut_thres && cur_bl_y > Y_STOP) {
      int mid = (y0 + y1) / 2;
      cilk_scope {
          cilk_spawn walk_dp_xyt(Q, t0, t1, x0, dx0, x1, dx1, y0, dy0, mid, -SLOPE_Y); // bottom left triangle
          cilk_spawn walk_dp_xyt(Q, t0, t1, x0, dx0, x1, dx1, mid, SLOPE_Y, y1, dy1); // bottom right triangle
      }
      walk_dp_xyt(Q, t0, t1, x0, dx0, x1, dx1, mid, -SLOPE_Y, mid, SLOPE_Y); // top mid triangle
      return;
    } else if (!ywellShaped && cur_ul_y > y_cut_thres
               && cur_ul_y > Y_STOP) { // top > bottom && top sufficiently large
      int mid = (y0 + dy0 * lt + y1 + dy1 * lt) / 2;

      int bLeft = mid - SLOPE_Y * lt;
      int bRight = mid + SLOPE_Y * lt;

      walk_dp_xyt(Q, t0, t1, x0, dx0, x1, dx1, bLeft, SLOPE_Y, bRight, -SLOPE_Y); // bottom mid triangle
      cilk_scope {
          cilk_spawn walk_dp_xyt(Q, t0, t1, x0, dx0, x1, dx1, y0, dy0, bLeft, SLOPE_Y); // top left triangle
          cilk_spawn walk_dp_xyt(Q, t0, t1, x0, dx0, x1, dx1, bRight, -SLOPE_Y, y1,
          dy1); // top right triangle
      }
      return;
    } else if (lt > DT_STOP) {
      int halflt = lt / 2;
      walk_dp_xyt(Q, t0, t0 + halflt, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
      walk_dp_xyt(Q, t0 + halflt, t1,
                  x0 + dx0 * halflt, dx0, x1 + dx1 * halflt, dx1,
                  y0 + dy0 * halflt, dy0, y1 + dy1 * halflt, dy1);
      return;
    }

  }
}

static inline void walk_dp_xy_ucut(const SimState* Q,
                                   int t0, int t1,
                                   int x0, int dx0, int x1, int dx1,
                                   int y0, int dy0, int y1, int dy1) {
  /* This is steve's original dual-partition version
   */
  int lt = t1 - t0;
  int cur_bl_x = x1 - x0;
  int cur_bl_y = y1 - y0;
  int x_cut_thres = 4 * SLOPE_X * lt;
  int y_cut_thres = 4 * SLOPE_Y * lt;
  /* By well-shaped, we mean bottom >= top, otherwise, it's !wellShaped
   */
  if (lt <= 1 || (cur_bl_x <= X_STOP && cur_bl_y <= Y_STOP && lt <= DT_STOP)) {
    Q->base_case_kernel(t0, t1, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
    return;
  } else  {
    if (lt <= DT_STOP && cur_bl_x > x_cut_thres) {
      int mid = (x0 + x1) / 2;
      cilk_scope {
          cilk_spawn walk_dp_xy_ucut(Q, t0, t1, x0, SLOPE_X, mid, -SLOPE_X, y0, dy0, y1, dy1);
          walk_dp_xy_ucut(Q, t0, t1, mid, SLOPE_X, x1, -SLOPE_X, y0, dy0, y1, dy1);
          cilk_sync;
          if (dx0 != SLOPE_X) {
            cilk_spawn walk_dp_xy_ucut(Q, t0, t1, x0, dx0, x0, SLOPE_X, y0, dy0, y1, dy1);
          }
          cilk_spawn walk_dp_xy_ucut(Q, t0, t1, mid, -SLOPE_X, mid, SLOPE_X, y0, dy0, y1, dy1);
          if (dx1 != -SLOPE_X) {
            cilk_spawn walk_dp_xy_ucut(Q, t0, t1, x1, -SLOPE_X, x1, dx1, y0, dy0, y1, dy1);
          }
      }
      return;
    } else if (lt <= DT_STOP && cur_bl_y > y_cut_thres) {
      int mid = (y0 + y1) / 2;
      cilk_scope {
          cilk_spawn walk_dp_xy_ucut(Q, t0, t1, x0, dx0, x1, dx1, y0, SLOPE_Y, mid, -SLOPE_Y);
          walk_dp_xy_ucut(Q, t0, t1, x0, dx0, x1, dx1, mid, SLOPE_Y, y1, -SLOPE_Y);
          cilk_sync;
          if (dy0 != SLOPE_Y) {
            cilk_spawn walk_dp_xy_ucut(Q, t0, t1, x0, dx0, x1, dx1, y0, dy0, y0, SLOPE_Y);
          }
          cilk_spawn walk_dp_xy_ucut(Q, t0, t1, x0, dx0, x1, dx1, mid, -SLOPE_Y, mid, SLOPE_Y);
          if (dy1 != -SLOPE_Y) {
            cilk_spawn walk_dp_xy_ucut(Q, t0, t1, x0, dx0, x1, dx1, y1, -SLOPE_Y, y1, dy1);
          }
      }
      return;
    } else if (lt > DT_STOP) {
      int halflt = lt / 2;
      walk_dp_xy_ucut(Q, t0, t0 + halflt, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
      walk_dp_xy_ucut(Q, t0 + halflt, t1,
                      x0 + dx0 * halflt, dx0, x1 + dx1 * halflt, dx1,
                      y0 + dy0 * halflt, dy0, y1 + dy1 * halflt, dy1);
      return;
    }

  }
}

static inline void walk_dp_xyt_ucut2(const SimState* Q,
                                     int t0, int t1,
                                     int x0, int dx0, int x1, int dx1,
                                     int y0, int dy0, int y1, int dy1) {
  /* This is steve's original dual-partition version
   */
  int lt = t1 - t0;
  int cur_bl_x = x1 - x0;
  int cur_bl_y = y1 - y0;
  int x_cut_thres = 4 * SLOPE_X * lt;
  int y_cut_thres = 4 * SLOPE_Y * lt;
  /* By well-shaped, we mean bottom >= top, otherwise, it's !wellShaped
   */
  if (cur_bl_x <= X_STOP && cur_bl_y <= Y_STOP && lt <= DT_STOP) {
    Q->base_case_kernel(t0, t1, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
    return;
  } else  {
    if (cur_bl_x > x_cut_thres && cur_bl_x > X_STOP) {
      //int mid = (x0 + x1) / 2;
      int third = (x1 - x0) / 3;
      cilk_scope {
          cilk_spawn walk_dp_xyt_ucut2(Q, t0, t1, x0, SLOPE_X, x0 + third, -SLOPE_X, y0, dy0, y1, dy1);
          walk_dp_xyt_ucut2(Q, t0, t1, x0 + third * 2, SLOPE_X, x1, -SLOPE_X, y0, dy0, y1, dy1);
          cilk_sync;
          if (dx0 != SLOPE_X) {
            cilk_spawn walk_dp_xyt_ucut2(Q, t0, t1, x0, dx0, x0, SLOPE_X, y0, dy0, y1, dy1);
          }
          cilk_spawn walk_dp_xyt_ucut2(Q, t0, t1, x0 + third, -SLOPE_X, x0 + third * 2, SLOPE_X, y0, dy0, y1,
          dy1);
          if (dx1 != -SLOPE_X) {
            cilk_spawn walk_dp_xyt_ucut2(Q, t0, t1, x1, -SLOPE_X, x1, dx1, y0, dy0, y1, dy1);
          }
          //std::cout << "something 1" << std::endl;
      }
      return;
    } else if (cur_bl_y > y_cut_thres && cur_bl_y > Y_STOP) {
      //int mid = (y0 + y1) / 2;
      int third = (y1 - y0) / 3;
      cilk_scope {
          cilk_spawn walk_dp_xyt_ucut2(Q, t0, t1, x0, dx0, x1, dx1, y0, SLOPE_Y, y0 + third, -SLOPE_Y);
          walk_dp_xyt_ucut2(Q, t0, t1, x0, dx0, x1, dx1, y0 + third * 2, SLOPE_Y, y1, -SLOPE_Y);
          cilk_sync;
          if (dy0 != SLOPE_Y) {
            cilk_spawn walk_dp_xyt_ucut2(Q, t0, t1, x0, dx0, x1, dx1, y0, dy0, y0, SLOPE_Y);
          }
          cilk_spawn walk_dp_xyt_ucut2(Q, t0, t1, x0, dx0, x1, dx1, y0 + third, -SLOPE_Y, y0 + third * 2,
          SLOPE_Y);
          if (dy1 != -SLOPE_Y) {
            cilk_spawn walk_dp_xyt_ucut2(Q, t0, t1, x0, dx0, x1, dx1, y1, -SLOPE_Y, y1, dy1);
          }
          //std::cout << "something 2" << std::endl;
      }
      return;
    } else if (lt > DT_STOP) {
      int halflt = lt / 2;
      walk_dp_xyt_ucut2(Q, t0, t0 + halflt, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
      walk_dp_xyt_ucut2(Q, t0 + halflt, t1,
                        x0 + dx0 * halflt, dx0, x1 + dx1 * halflt, dx1,
                        y0 + dy0 * halflt, dy0, y1 + dy1 * halflt, dy1);
      return;
    }

  }
}

static inline void walk_dp_xyt_ucut(const SimState* Q,
                                    int t0, int t1,
                                    int x0, int dx0, int x1, int dx1,
                                    int y0, int dy0, int y1, int dy1) {
  /* This is steve's original dual-partition version
   */
  int lt = t1 - t0;
  int cur_bl_x = x1 - x0;
  int cur_bl_y = y1 - y0;
  int x_cut_thres = 4 * SLOPE_X * lt;
  int y_cut_thres = 4 * SLOPE_Y * lt;
  /* By well-shaped, we mean bottom >= top, otherwise, it's !wellShaped
   */
  if (cur_bl_x <= X_STOP && cur_bl_y <= Y_STOP && lt <= DT_STOP) {
    Q->base_case_kernel(t0, t1, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
    return;
  } else  {
    if (cur_bl_x > x_cut_thres && cur_bl_x > X_STOP) {
      int mid = (x0 + x1) / 2;
      cilk_scope {
          cilk_spawn walk_dp_xyt_ucut(Q, t0, t1, x0, SLOPE_X, mid, -SLOPE_X, y0, dy0, y1, dy1);
          walk_dp_xyt_ucut(Q, t0, t1, mid, SLOPE_X, x1, -SLOPE_X, y0, dy0, y1, dy1);
          cilk_sync;
          if (dx0 != SLOPE_X) {
            cilk_spawn walk_dp_xyt_ucut(Q, t0, t1, x0, dx0, x0, SLOPE_X, y0, dy0, y1, dy1);
          }
          cilk_spawn walk_dp_xyt_ucut(Q, t0, t1, mid, -SLOPE_X, mid, SLOPE_X, y0, dy0, y1, dy1);
          if (dx1 != -SLOPE_X) {
            cilk_spawn walk_dp_xyt_ucut(Q, t0, t1, x1, -SLOPE_X, x1, dx1, y0, dy0, y1, dy1);
          }
      }
      return;
    } else if (cur_bl_y > y_cut_thres && cur_bl_y > Y_STOP) {
      int mid = (y0 + y1) / 2;
      cilk_scope {
          cilk_spawn walk_dp_xyt_ucut(Q, t0, t1, x0, dx0, x1, dx1, y0, SLOPE_Y, mid, -SLOPE_Y);
          walk_dp_xyt_ucut(Q, t0, t1, x0, dx0, x1, dx1, mid, SLOPE_Y, y1, -SLOPE_Y);
          cilk_sync;
          if (dy0 != SLOPE_Y) {
            cilk_spawn walk_dp_xyt_ucut(Q, t0, t1, x0, dx0, x1, dx1, y0, dy0, y0, SLOPE_Y);
          }
          cilk_spawn walk_dp_xyt_ucut(Q, t0, t1, x0, dx0, x1, dx1, mid, -SLOPE_Y, mid, SLOPE_Y);
          if (dy1 != -SLOPE_Y) {
            cilk_spawn walk_dp_xyt_ucut(Q, t0, t1, x0, dx0, x1, dx1, y1, -SLOPE_Y, y1, dy1);
          }
      }
      return;
    } else if (lt > DT_STOP) {
      int halflt = lt / 2;
      walk_dp_xyt_ucut(Q, t0, t0 + halflt, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
      walk_dp_xyt_ucut(Q, t0 + halflt, t1,
                       x0 + dx0 * halflt, dx0, x1 + dx1 * halflt, dx1,
                       y0 + dy0 * halflt, dy0, y1 + dy1 * halflt, dy1);
      return;
    }

  }
}


static inline void walk_dp_xyt_ucut_fixed(const SimState* Q,
                                          int t0, int t1,
                                          int x0, int dx0, int x1, int dx1,
                                          int y0, int dy0, int y1, int dy1) {
  /* This is steve's original dual-partition version
   */
  int lt = t1 - t0;
  int cur_bl_x = x1 - x0;
  int cur_bl_y = y1 - y0;
  int x_cut_thres = 4 * SLOPE_X * lt;
  int y_cut_thres = 4 * SLOPE_Y * lt;

  if (cur_bl_x <= X_STOP && cur_bl_y <= Y_STOP && lt <= DT_STOP) {
    Q->base_case_kernel(t0, t1, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
    return;
  } else  {
    if (cur_bl_x > x_cut_thres && cur_bl_x >= 2 * X_STOP) {
      int mid = (x0 + x1) / 2;
      debug("cut into mid-X\n", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      cilk_scope {
          cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, SLOPE_X, mid, -SLOPE_X, y0, dy0, y1, dy1);
          walk_dp_xyt_ucut_fixed(Q, t0, t1, mid, SLOPE_X, x1, -SLOPE_X, y0, dy0, y1, dy1);
          cilk_sync;
          if (dx0 != SLOPE_X) {
            cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, dx0, x0, SLOPE_X, y0, dy0, y1, dy1);
          }
          cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, mid, -SLOPE_X, mid, SLOPE_X, y0, dy0, y1, dy1);
          if (dx1 != -SLOPE_X) {
            cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x1, -SLOPE_X, x1, dx1, y0, dy0, y1, dy1);
          }
      }
      return;
    } else if (cur_bl_x > x_cut_thres && cur_bl_x > X_STOP && cur_bl_x < 2 * X_STOP) {
      assert(cur_bl_x < 2 * X_STOP);
      debug("cut into 100-block X\n", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      debug("ucut_fixed1", t0, t1, x0, SLOPE_X, x0 + X_STOP, -SLOPE_X, y0, dy0, y1, dy1);
      cilk_scope {
          cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, SLOPE_X, x0 + X_STOP, -SLOPE_X, y0, dy0, y1, dy1);
          cilk_sync;
          debug("ucut_fixed2", t0, t1, x0, dx0, x0, SLOPE_X, y0, dy0, y1, dy1);
          cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, dx0, x0, SLOPE_X, y0, dy0, y1, dy1);
          debug("ucut_fixed3", t0, t1, x0 + X_STOP, -SLOPE_X, x1, dx1, y0, dy0, y1, dy1);
          cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0 + X_STOP, -SLOPE_X, x1, dx1, y0, dy0, y1, dy1);
      }
      return;
    } else if (cur_bl_y > y_cut_thres && cur_bl_y >= 2 * Y_STOP) {
      int mid = (y0 + y1) / 2;
      debug("cut into mid-Y", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      cilk_scope {
          cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, dx0, x1, dx1, y0, SLOPE_Y, mid, -SLOPE_Y);
          walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, dx0, x1, dx1, mid, SLOPE_Y, y1, -SLOPE_Y);
          cilk_sync;
          if (dy0 != SLOPE_Y) {
            cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, dx0, x1, dx1, y0, dy0, y0, SLOPE_Y);
          }
          cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, dx0, x1, dx1, mid, -SLOPE_Y, mid, SLOPE_Y);
          if (dy1 != -SLOPE_Y) {
            cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, dx0, x1, dx1, y1, -SLOPE_Y, y1, dy1);
          }
      }
      return;
    } else if (cur_bl_y > y_cut_thres && cur_bl_y > Y_STOP && cur_bl_y < 2 * Y_STOP) {
      assert(cur_bl_y < 2 * Y_STOP);
      debug("cut into 100-block Y", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      debug("ucut_fixed4", t0, t1, x0, dx0, x1, dx1, y0, SLOPE_Y, y0 + Y_STOP, -SLOPE_Y);
      cilk_scope {
          cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, dx0, x1, dx1, y0, SLOPE_Y, y0 + Y_STOP, -SLOPE_Y);
          cilk_sync;
          debug("ucut_fixed5", t0, t1, x0, dx0, x1, dx1, y0, dy0, y0, SLOPE_Y);
          cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, dx0, x1, dx1, y0, dy0, y0, SLOPE_Y);
          debug("ucut_fixed6", t0, t1, x0, dx0, x1, dx1, y0 + Y_STOP, -SLOPE_Y, y1, dy1);
          cilk_spawn walk_dp_xyt_ucut_fixed(Q, t0, t1, x0, dx0, x1, dx1, y0 + Y_STOP, -SLOPE_Y, y1, dy1);
      }
      return;
    } else if (lt > DT_STOP) {
      int halflt = lt / 2;
      walk_dp_xyt_ucut_fixed(Q, t0, t0 + halflt, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
      walk_dp_xyt_ucut_fixed(Q, t0 + halflt, t1,
                             x0 + dx0 * halflt, dx0, x1 + dx1 * halflt, dx1,
                             y0 + dy0 * halflt, dy0, y1 + dy1 * halflt, dy1);
      return;
    }

  }
}


void walk_dp_cq(const SimState* Q,
                int t0, int t1,
                int x0, int dx0, int x1, int dx1,
                int y0, int dy0, int y1, int dy1) {
  /* This is steve's original dual-partition version
   */
  int lt = t1 - t0;
  int cur_bl_x = x1 - x0;
  int cur_bl_y = y1 - y0;
  int x_cut_thres = 4 * SLOPE_X * lt;
  int y_cut_thres = 4 * SLOPE_Y * lt;
  if ((cur_bl_x <= X_STOP || cur_bl_x < x_cut_thres) && (cur_bl_y <= Y_STOP
                                                         || cur_bl_y < y_cut_thres) && lt <= DT_STOP) {
    for (int i = t0; i < t1; i++) {
      Q->kernel_single_timestep(i, x0, x1, y0, y1);
      x0 += dx0;
      x1 += dx1;
      y0 += dy0;
      y1 += dy1;
    }
    return;
  } else {
    if (cur_bl_x >= x_cut_thres) {
      /* cut into X dimension */
      int mid = (x0 + x1) / 2;
      int bLeft = mid - SLOPE_X * lt;
      int bRight = mid + SLOPE_X * lt;
      cilk_scope {
          cilk_spawn walk_dp_cq(Q, t0, t1, x0, dx0, bRight, -SLOPE_X, y0, dy0, y1, dy1);
          cilk_spawn walk_dp_cq(Q, t0, t1, bLeft, SLOPE_X, x1, dx1, y0, dy0, y1, dy1);
      }
      return;
    } else if (cur_bl_y >= y_cut_thres) {
      /* cut into Y dimension */
      int mid = (y0 + y1) / 2;
      int bLeft = mid - SLOPE_Y * lt;
      int bRight = mid + SLOPE_Y * lt;
      cilk_scope {
          cilk_spawn walk_dp_cq(Q, t0, t1, x0, dx0, x1, dx1, y0, dy0, bRight, -SLOPE_Y);
          cilk_spawn walk_dp_cq(Q, t0, t1, x0, dx0, x1, dx1, bLeft, SLOPE_Y, y1, dy1);
      }
      return;
    } else {
      int halflt = lt / 2;
      walk_dp_cq(Q, t0, t0 + halflt, x0, dx0, x1, dx1, y0, dy0, y1, dy1);
      walk_dp_cq(Q, t0 + halflt, t1,
                 x0 + dx0 * halflt, dx0, x1 + dx1 * halflt, dx1,
                 y0 + dy0 * halflt, dy0, y1 + dy1 * halflt, dy1);
    }

  }
}

//
//void rect_recursive_dp_t_(void* args) {
//  argBundle* data = (argBundle*)args;
//  walk_dp_t(data->Q,
//            data->t0, data->t1,
//            data->x0, 0, data->x1, 0,
//            data->y0, 0, data->y1, 0);
//}
//void rect_recursive_dp_ucut_(void* args) {
//  argBundle* data = (argBundle*)args;
//  walk_dp_xyt_ucut(data->Q,
//                   data->t0, data->t1,
//                   data->x0, 0, data->x1, 0,
//                   data->y0, 0, data->y1, 0);
//}
//void rect_recursive_dp_ucut_fixed_(void* args) {
//  argBundle* data = (argBundle*)args;
//  walk_dp_xyt_ucut_fixed(data->Q,
//                         data->t0, data->t1,
//                         data->x0, 0, data->x1, 0,
//                         data->y0, 0, data->y1, 0);
//}
//void rect_recursive_dp_xyt_(void* args) {
//  argBundle* data = (argBundle*)args;
//  walk_dp_xyt(data->Q,
//              data->t0, data->t1,
//              data->x0, 0, data->x1, 0,
//              data->y0, 0, data->y1, 0);
//}
//void rect_recursive_dp_xy_(void* args) {
//  argBundle* data = (argBundle*)args;
//  walk_dp_xy_ucut(data->Q,
//                  data->t0, data->t1,
//                  data->x0, 0, data->x1, 0,
//                  data->y0, 0, data->y1, 0);
//}
//void rect_recursive_dp_xyt_ucut2_(void* args) {
//  argBundle* data = (argBundle*)args;
//  walk_dp_xyt_ucut2(data->Q,
//                    data->t0, data->t1,
//                    data->x0, 0, data->x1, 0,
//                    data->y0, 0, data->y1, 0);
//}
/*void rect_recursive_(void* args)
{
  argBundle *data = (argBundle*)args;
  walk_dp_xyt_ucut2(data->Q,
      data->t0, data->t1,
      data->x0, 0, data->x1, 0,
      data->y0, 0, data->y1, 0);
}*/

void rect_recursive_dp_ucut(const SimState* Q,
                            int t0, int t1,
                            int x0, int x1,
                            int my_y0, int my_y1) {
  walk_dp_xyt_ucut(Q,
                   t0, t1,
                   x0, 0, x1, 0,
                   my_y0, 0, my_y1, 0);
}
