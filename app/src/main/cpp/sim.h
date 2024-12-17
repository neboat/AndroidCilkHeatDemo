/* Copyright (C) 2010 Zhunping Zhang
 *
 * Code for 6.884 Lab 3
 *
 * Modified by Jim Sukha, February 2010
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
#ifndef CILKHEATDEMO2_SIM_H
#define CILKHEATDEMO2_SIM_H

#include "common.h"

/**************************************************/
// Block size parameter.
#ifdef BLOCK_VALUE
#define LG_B BLOCK_VALUE
#else
#define LG_B 1
#endif

const int BLOCK_DIM = (1 << LG_B);
const int LOW_MASK = (BLOCK_DIM - 1);
const int HIGH_MASK = (~LOW_MASK);

#define S_BLOCK_IDX(x) ((x) & HIGH_MASK)
#define S_IDX(x) ((x) & LOW_MASK)
#define ColMajorBOffset(Q, x, y) (((Q)->Ysep)*S_BLOCK_IDX(x) + BLOCK_DIM*(S_BLOCK_IDX(y) + S_IDX(x)) + S_IDX(y))
#define RowMajorBOffset(Q, x, y) (((Q)->Xsep)*S_BLOCK_IDX(y) + BLOCK_DIM*(S_BLOCK_IDX(x) + S_IDX(y)) + S_IDX(x))

// For an X by Y grid, the number of elements to allocate space
#define BlockRound(X) (S_BLOCK_IDX(X) + BLOCK_DIM*(S_IDX(X)>0))
#define GridSize(X, Y) (BlockRound(X) * BlockRound(Y))

// variables for actual computation
#define DEFAULT_TSTEP 175
#define REAL_TIME_PER_TSTEP 0.00000175f

#define Xmul 0.03            //X0,Y0,X1,Y1,T0,T1 tells the actual physical size of the grid,  X1 = X * Xmul
#define Ymul 0.03

const double alpha_val = 0.05;

const double X0 = 0.0;
const double Y0 = 0.0;
const double T0 = 0.0;
const double alpha = alpha_val;

// Note: when defining preprocessor macros,
// make sure to include parentheses around
// inputs.
//
// Otherwise, if you pass in something like "t" is "q+1",
// t&1  will get translated into q+1&1, which
// will give the wrong answer.
//
// Various ways of indexing the array.
#define RowMajorIdxTInner(Q, t, x, y) (2*((Q)->Xsep*(y) + x) + ((t)&1))
#define ColMajorIdxTInner(Q, t, x, y) (2*((Q)->Ysep*(x) + y) + ((t)&1))
#define RowMajorIdxTOuter(Q, t, x, y) ((Q)->Xsep*((Q)->Ysep)*((t)&1) + ((Q)->Xsep*(y) + x))
#define ColMajorIdxTOuter(Q, t, x, y) ((Q)->Xsep*((Q)->Ysep)*((t)&1) + ((Q)->Ysep*(x) + y))
#define RowMajorIdxTBlockInner(Q, t, x, y) (2*(RowMajorBOffset(Q, x, y)) + ((t)&1))
#define ColMajorIdxTBlockInner(Q, t, x, y) (2*(ColMajorBOffset(Q, x, y)) + ((t)&1))

#if (LG_B == 1)
#define Idx(Q, t, x, y) RowMajorIdxTInner(Q, t, x, y)
#else
#define Idx(Q, t, x, y) RowMajorIdxTBlockInner(Q, t, x, y)
#endif

#define U(Q, t, x, y) (Q)->u[Idx(Q, t, x, y)]
#define Uaddr(Q, t, x, y) ((Q)->u +  Idx(Q, t, x, y))
#define Raster(Q, y, x) ((Q)->raster[(Q)->Ysep * (x) + (y)])

// Data structure which holds the simulation state.
// To compare with the Lab 3 handout,
//  X and Y correspond to M and L from the lab handout
//  DT, DX, DY correspond to \DeltaT, \DeltaX, and \DeltaY
class SimState {
public:
  int X = 0;  // Dimensions of the grid for the simulation.
  int Y = 0;
  int TStep = 0;

  // Derive these arguments from X, Y, and TStep.
  double X1 = 0.0;  // Conceptual size of the grid.
  double Y1 = 0.0;
  double T1 = 0.0;
  double DT = 0.0, DX = 0.0, DY = 0.0;

  double CX = 0.0;  // CX = alpha * DT / DX^2
  double CY = 0.0;  // CY = alpha * DT / DY^2

  int Xsep = 0;    // Dimensions of the array to allocate.
  int Ysep = 0;    // This may be rounded up to nearst block size multiple.

  double *u = nullptr; // the 2*Xsep*Ysep array to store the values.
  char *raster = nullptr;  //here is the heat source pattern

  float heat_inc = 0.0;  //heat increased at each positive point in the pattern

  SimState(int x_sep, int y_sep, bool zero_init)
      : Xsep(BlockRound(x_sep)), Ysep(BlockRound(y_sep)) {
    if (zero_init) {
      u = (double *) calloc(GridSize(x_sep, y_sep) * 2, sizeof(double));
      raster = (char *) calloc(GridSize(x_sep, y_sep), sizeof(char));
    } else {
      u = (double *) malloc(GridSize(x_sep, y_sep) * 2 * sizeof(double));
      raster = (char *) malloc(GridSize(x_sep, y_sep) * sizeof(char));
    }
  }

  ~SimState() {
    free(u);
    free(raster);
  }

  void clear_raster_array() const {
    memset(raster, 0, GridSize(Xsep, Ysep) * sizeof(char));
  }

  void clear() const {
    clear_raster_array();
    memset(u, 0, GridSize(Xsep, Ysep) * sizeof(double));
  }

  // Takes values of X, Y, and TStep from params,
  // and sets appropriate values in Q.
  void set_sim_size(int X, int Y, int TStep) {
    this->X = X;
    this->Y = Y;
    this->TStep = TStep;

    // Derive the rest of the arguments
    X1 = X * Xmul;
    Y1 = Y * Ymul;
    DX = (X1 - X0) / (X - 1);
    DY = (Y1 - Y0) / (Y - 1);

    // Jim: I believe we need something like
    // 1 - 2*CX - 2*CY > 0 for stability?
    //
    // (I believe closer to 1 is more accurate?)
    //
    DT = (0.125 / alpha) * min((DX * DX), (DY * DY));
    T1 = T0 + DY;
    CX = alpha * DT / (DX * DX);
    CY = alpha * DT / (DY * DY);
  }

  // Applies the kernel for a single timestep.
  void kernel_inline(int t, int x, int y) const {
    if (x == 0 || x == X - 1 || y == 0 || y == Y - 1) {
      U(this, t + 1, x, y) = 0.0;
    } else
      U(this, t + 1, x, y) =
          alpha * (CX * (U(this, t, x + 1, y) - 2.0 * U(this, t, x, y) + U(this, t, x - 1, y))
                   + CY * (U(this, t, x, y + 1) - 2.0 * U(this, t, x, y) + U(this, t, x, y - 1)))
          + U(this, t, x, y);

    // add the heat
    U(this, t + 1, x, y) += heat_inc * Raster(this, y, x);
  }

  void null_kernel(int t, int x, int y) const {
    U(this, t + 1, x, y) += heat_inc * Raster(this, y, x);
  }

  void rect_null(int t0, int t1,
                 int x0, int x1,
                 int my_y0, int my_y1) const {
    for (int t = t0; t < t1; t++) {
      for (int x = x0; x < x1; x++) {
        int y;
        for (y = my_y0; y + 7 < my_y1; y += 8) {
          null_kernel(t, x, y);
          null_kernel(t, x, y + 1);
          null_kernel(t, x, y + 2);
          null_kernel(t, x, y + 3);
          null_kernel(t, x, y + 4);
          null_kernel(t, x, y + 5);
          null_kernel(t, x, y + 6);
          null_kernel(t, x, y + 7);
        }
        for (; y < my_y1; y++) {
          null_kernel(t, x, y);
        }
      }
    }
  }
  // Jim: This is only here because Cilk++ crashes if LG_B > 1
  // In this case, we use the more complicated indexing macro, and
  // maybe Cilk++ can not inline that much?
#if (LG_B == 1)
#define kernel kernel_inline
#else
  static void kernel_no_inline(int t, int x, int y) {
    kernel_inline(t, x, y);
  }
#define kernel kernel_no_inline
#endif

  void kernel_single_timestep(int t, int my_x0, int my_x1, int my_y0, int my_y1) const {
    for (int x = my_x0; x < my_x1; ++x) {
      for (int y = my_y0; y < my_y1; ++y) {
        kernel(t, x, y);
      }
    }
  }

  void base_case_kernel(int t0, int t1, int x0, int dx0, int x1,
                        int dx1, int y0, int dy0, int y1, int dy1) const {
    for (int t = t0; t < t1; t++) {
      kernel_single_timestep(t, x0, x1, y0, y1);
      /* because the shape is trapezoid */
      x0 += dx0;
      x1 += dx1;
      y0 += dy0;
      y1 += dy1;
    }
  }
};

#endif //CILKHEATDEMO2_SIM_H
