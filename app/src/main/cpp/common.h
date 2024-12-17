/* Copyright (C) 2010 Zhunping Zhang
 *
 * Code for 6.884 Lab 3
 *
 * Modified by Jim Sukha, February 2010
 * Ported to Android by Tao B. Schardl, December 2024
 *
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

#ifndef COMMON_H
#define COMMON_H

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/time.h>
#include <cilk/cilk.h>

#define min(x, y)  (x<y?x:y)
#define max(x, y)  (x>y?x:y)

class SimState;

//#define TexImage(Q, x, y, z) (texImage[(4 * (((Q)->Ysep * (x)) + (y))) + z])
//#define TexImage(Q, x, y, z) (texImage[(4 * (((Q)->Xsep * ((Q)->Y - 1 - (y))) + (x))) + z])
#define TexImage(Q, x, y, z) (texImage[(4 * (((Q)->Xsep * (y)) + (x))) + z])

/*********************************************************/
// Structs and wrapper functions required to call Cilk++ code
// from within a C++ function.

void rect_recursive_serial(const SimState *Q,
                           int t0, int t1,
                           int x0, int x1,
                           int y0, int y1);

void rect_recursive_dp_ucut(const SimState *Q,
                            int t0, int t1,
                            int x0, int x1,
                            int y0, int y1);

void rect_loops_serial(const SimState *Q,
                       int t0, int t1,
                       int x0, int x1,
                       int y0, int y1);

void rect_loops_parallel(const SimState *Q,
                         int t0, int t1,
                         int x0, int x1,
                         int y0, int y1);

#endif
