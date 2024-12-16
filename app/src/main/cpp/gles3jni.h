/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GLES3JNI_H
#define GLES3JNI_H 1

#include <android/log.h>
#include <cmath>
#include "common.h"
#include "sim.h"

#if DYNAMIC_ES3
#include "gl3stub.h"
#else
// Include the latest possible header file( GL version header )
#if __ANDROID_API__ >= 24
#include <GLES3/gl32.h>
#elif __ANDROID_API__ >= 21
#include <GLES3/gl31.h>
#else
#include <GLES3/gl3.h>
#endif

#endif

#define DEBUG 1

#define LOG_TAG "GLES3JNI"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#if DEBUG
#define ALOGV(...) \
  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#else
#define ALOGV(...)
#endif

// ----------------------------------------------------------------------------
// Types, functions, and data used by both ES2 and ES3 renderers.
// Defined in gles3jni.cpp.

#define MAX_INSTANCES_PER_SIDE 4
#define MAX_INSTANCES (MAX_INSTANCES_PER_SIDE * MAX_INSTANCES_PER_SIDE)
#define TWO_PI (2.0 * M_PI)
#define MAX_ROT_SPEED (0.3 * TWO_PI)

//#define TexImage(Q, x, y, z) (texImage[(4 * (((Q)->Ysep * (x)) + (y))) + z])
#define TexImage(Q, x, y, z) (texImage[(4 * (((Q)->Xsep * ((Q)->Y - 1 - (y))) + (x))) + z])

// This demo uses three coordinate spaces:
// - The model (a quad) is in a [-1 .. 1]^2 space
// - Scene space is either
//    landscape: [-1 .. 1] x [-1/(2*w/h) .. 1/(2*w/h)]
//    portrait:  [-1/(2*h/w) .. 1/(2*h/w)] x [-1 .. 1]
// - Clip space in OpenGL is [-1 .. 1]^2
//
// Conceptually, the quads are rotated in model space, then scaled (uniformly)
// and translated to place them in scene space. Scene space is then
// non-uniformly scaled to clip space. In practice the transforms are combined
// so vertices go directly from model to clip space.

struct Vertex {
  GLfloat pos[2];
//  GLubyte rgba[4];
  GLfloat texCoord[2];
};
extern const Vertex QUAD[4];
extern const GLuint indices[6];

// returns true if a GL error occurred
extern bool checkGlError(const char* funcName);
extern GLuint createShader(GLenum shaderType, const char* src);
extern GLuint createProgram(const char* vtxSrc, const char* fragSrc);

// ----------------------------------------------------------------------------
// Interface to the ES2 and ES3 renderers, used by JNI code.

class Renderer {
 public:
  virtual ~Renderer();
  void resize(int w, int h);
  void render();

  void touchXY(float x, float y) {
    if (!Q)
      return;
//    ALOGV("touchXY: %f, %f\n", x, y);
//    float Xscale = float(Q->X) / winW;
//    float Yscale = float(Q->Y) / winH;
//    ALOGV("Xscale %f, Yscale %f\n", Xscale, Yscale);
    hot = 1;
    hx = (int)((float)x * Xscale);
    hy = (int)((float)y * Yscale);
    hx = min(max(hx, 0), Q->X - 1);
    hy = min(max(hy, 0), Q->Y - 1);
//    ALOGV("x %f, y %f, hx %d, hy %d, winW %d, winH %d, Q->X %d, Q->Y %d\n",
//          x, y, hx, hy, winW, winH, Q->X, Q->Y);
    // record the trail of mouse
    hxs[recording][nsegs[recording]] = hx;
    hys[recording][nsegs[recording]] = hy;
    nsegs[recording]++;
  }

  void releaseXY() {
//    ALOGV("releaseXY\n");
    hot = 0;
  }

 protected:
  enum { VB_INSTANCE, /*VB_SCALEROT, VB_OFFSET,*/ VB_COUNT };
  GLuint mProgram;
  GLuint texName;
  GLubyte *texImage = NULL;
  GLuint mVB[VB_COUNT];
  GLuint mVBState;
  GLuint EBO;

  SimState* Q = NULL;
  long t = 0;

  const float MUL = 2.8 * 2;
  int reshape = 0, rx, ry;  // some tricks
  int drawing = 0;          // avoiding reentering race
  // float divx = MUL, divy = MUL;  // help maps mouse location from window
  float Xscale = 1, Yscale = 1;
  // coordinates to grid coordinates

  int winW = 1, winH = 1;
  int hot = 1;

  // Jim: This display code I think makes some assumption of the size of the
  // window that is it less than a certain size?  This part needs to be fixed to
  // handle more general grids.
  int hx = 0, hy = 0;              // current heat source location
  int hxs[2][1024], hys[2][1024];  // heat source trail
  int nsegs[2] = {0,0};    // heat source trail #segments
  int recording = 0;  // which trail are we recording.  here we use a double buffer trick.
  const float total_heat_per_frame = 0.2;

  Renderer();

  // return a pointer to a buffer of MAX_INSTANCES * sizeof(vec2).
  // the buffer is filled with per-instance offsets, then unmapped.
  virtual float* mapOffsetBuf() = 0;
  virtual void unmapOffsetBuf() = 0;
  // return a pointer to a buffer of MAX_INSTANCES * sizeof(vec4).
  // the buffer is filled with per-instance scale and rotation transforms.
  virtual float* mapTransformBuf() = 0;
  virtual void unmapTransformBuf() = 0;

  virtual void draw(unsigned int numInstances) = 0;

 private:
  void calcSceneParams(unsigned int w, unsigned int h, float* offsets);
  void step();

  unsigned int mNumInstances;
  float mScale[2];
  float mAngularVelocity[MAX_INSTANCES];
  uint64_t mLastFrameNs;
  float mAngles[MAX_INSTANCES];

  void draw_pixel(int x, int y) {
    Raster(Q, y, x) = hot;
  }
  int bres(int x1, int y1, int x2, int y2) {
    int dx, dy, i, e;
    int incx, incy, inc1, inc2;
    int x, y;

    dx = x2 - x1;
    dy = y2 - y1;

    if (dx < 0) {
      dx = -dx;
    }
    if (dy < 0) {
      dy = -dy;
    }
    incx = 1;
    if (x2 < x1) {
      incx = -1;
    }
    incy = 1;
    if (y2 < y1) {
      incy = -1;
    }
    x = x1;
    y = y1;

    if (dx > dy) {
      draw_pixel(x, y);
      e = 2 * dy - dx;
      inc1 = 2 * (dy - dx);
      inc2 = 2 * dy;
      for (i = 0; i < dx; i++) {
        if (e >= 0) {
          y += incy;
          e += inc1;
        } else {
          e += inc2;
        }
        x += incx;
        draw_pixel(x, y);
      }
      return dx;
    } else {
      draw_pixel(x, y);
      e = 2 * dx - dy;
      inc1 = 2 * (dx - dy);
      inc2 = 2 * dx;
      for (i = 0; i < dy; i++) {
        if (e >= 0) {
          x += incx;
          e += inc1;
        } else {
          e += inc2;
        }
        y += incy;
        draw_pixel(x, y);
      }
      return dy;
    }
  }
};

extern Renderer* createES2Renderer();
extern Renderer* createES3Renderer();

#endif  // GLES3JNI_H
