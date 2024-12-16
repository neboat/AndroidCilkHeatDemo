/* Copyright (C) 2010 Zhunping Zhang
 *
 * Code for 6.884 Lab 3
 *
 * Modified by Jim Sukha, February 2010
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

#ifndef COMMON_H
#define COMMON_H

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>

#include <cilk/cilk.h>
// #include <cilk/cilk_api.h>

#define min(x,y)  (x<y?x:y)
#define max(x,y)  (x>y?x:y)


/**************************************************/
// Block size parameter.
#ifdef BLOCK_VALUE
  #define LG_B BLOCK_VALUE
#else
  #define LG_B 1
#endif


#ifdef IS_CILKPP
  const int BLOCK_DIM = (1 << LG_B);
  const int BSQUARED  = (BLOCK_DIM* BLOCK_DIM);
  const int LOW_MASK = (BLOCK_DIM - 1);
  const int HIGH_MASK = (~LOW_MASK);
#else

  #define BLOCK_DIM  (1 << LG_B)
  #define BSQUARED (BLOCK_DIM*BLOCK_DIM)
  #define LOW_MASK (BLOCK_DIM-1)
  #define HIGH_MASK (~LOW_MASK)

#endif
#define S_BLOCK_IDX(x) ((x) & HIGH_MASK)
#define S_IDX(x) ((x) & LOW_MASK)
#define ColMajorBOffset(Q, x, y) (((Q)->Ysep)*S_BLOCK_IDX(x) + BLOCK_DIM*(S_BLOCK_IDX(y) + S_IDX(x)) + S_IDX(y))
#define RowMajorBOffset(Q, x, y) (((Q)->Xsep)*S_BLOCK_IDX(y) + BLOCK_DIM*(S_BLOCK_IDX(x) + S_IDX(y)) + S_IDX(x))

// For an X by Y grid, the number of elements to allocate space
#define BlockRound(X) (S_BLOCK_IDX(X) + BLOCK_DIM*(S_IDX(X)>0))
#define GridSize(X, Y) (BlockRound(X) * BlockRound(Y))

// variables for actual computation
#define DEFAULT_N BlockRound(300)        //grid size
#define DEFAULT_TSTEP 175
#define REAL_TIME_PER_TSTEP 0.00000175f

#define TIER 1               //how many chunks to compute between every display, a chunk consists of max(x,y) iterations


extern int CurrentP;   // Store the current number of workers we are running on.

#define Xmul 0.03            //X0,Y0,X1,Y1,T0,T1 tells the actual physical size of the grid,  X1 = X * Xmul
#define Ymul 0.03

const float alpha_val = 0.05;

#ifdef IS_CILKPP
  const double X0 = 0.0;
  const double Y0 = 0.0;
  const double T0 = 0.0;
  const double alpha = alpha_val;
#else
const float X0 = 0.0;
const float Y0 = 0.0;
const float T0 = 0.0;
const float alpha = alpha_val;
#endif

//// Data structure which holds the simulation state.
//// To compare with the Lab 3 handout,
////  X and Y correspond to M and L from the lab handout
////  DT, DX, DY correspond to \DeltaT, \DeltaX, and \DeltaY
//typedef struct {
//  int X;  // Dimensions of the grid for the simulation.
//  int Y;  //
//  int TStep;
//
//  // Derive these arguments from X, Y, and TStep.
//  double X1;  // Conceptual size of the grid.
//  double Y1;
//  double T1;
//  double DT, DX, DY;
//
//  double CX;  // CX = alpha * DT / DX^2
//  double CY;  // CY = alpha * DT / DY^2
//
//  int Xsep;    // Dimensions of the array to allocate.
//  int Ysep;    // This may be rounded up to nearst block size multiple.
//
//  double* u; // the 2*Xsep*Ysep array to store the values.
//  char* raster;  //here is the heat source pattern
//
//  float heat_inc;  //heat increased at each positive point in the pattern
//} SimState;


/// typedef struct SimState_s SimState;

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

class SimState;

/*********************************************************/
// Structs and wrapper functions required to call Cilk++ code
// from within a C++ function.

//// We use this struct to wrap arguments, so that Cilk++ code can be
//// called from a C++ function.
//typedef struct {
//  const SimState* Q;
//  int t0;
//  int t1;
//  int x0;
//  int x1;
//  int y0;
//  int y1;
//} argBundle;

// Jim: For the Cilk version, these are defined in heat_wrappers.cilkh now
// (since we can't mix cilkh declarations and normal .h declarations
// easily).

void rect_loops_(void* args);
void rect_recursive_(void* args);
void rect_loops_parallel_(void* args);
void rect_recursive_dp_ucut_(void* args);

void rect_recursive_serial(const SimState* Q,
                           int t0, int t1,
                           int x0, int x1,
                           int y0, int y1);

void rect_recursive_dp_ucut(const SimState* Q,
                            int t0, int t1,
                            int x0, int x1,
                            int y0, int y1);

void rect_loops_serial(const SimState* Q,
                       int t0, int t1,
                       int x0, int x1,
                       int y0, int y1);

void rect_loops_parallel(const SimState* Q,
                         int t0, int t1,
                         int x0, int x1,
                         int y0, int y1);

/*********************************************************/
// Other miscellaneous utility methods.

typedef struct {
  int X;
  int Y;
  int TStep;
  bool run_tests;
  bool no_loop;
  bool no_rec;
  bool no_ploop;
  bool no_prec;
} SimArgs;



// Method which extracts values from command line arguments.
SimArgs parse_arguments(int argc, char* argv[]);


// Dynamically create an simulation array for an (x_sep by y_sep) grid.
// If zero_init is true, then the simulation arrays are initialized to
// zero; otherwise, the memory may be uninitialized.
SimState* create_sim_state(int x_sep, int y_sep, bool zero_init);

// Frees the allocated array.
void destroy_sim_state(SimState* Q);


// Sets u and raster arrays both to 0.
void clear_sim_array(SimState* Q);

// Zeroes out the raster array.
void clear_raster_array(SimState* Q);

// Given the parameters in "params" (X, Y, and TStep),
// calculates all the constants for the simulation.
void set_sim_size(SimState* Q, SimArgs* params);

// Allocates a new array which is a copy of the input.
SimState* clone_sim_state(SimState* Q);

void print_sim_state(SimState* Q, bool print_borders);


static inline double tdiff(struct timeval* a, struct timeval* b)
// Effect: Compute the difference of b and a, that is $b-a$
//  as a double with units in seconds
{
  return a->tv_sec - b->tv_sec + 1e-6 * (a->tv_usec - b->tv_usec);
}

#endif
