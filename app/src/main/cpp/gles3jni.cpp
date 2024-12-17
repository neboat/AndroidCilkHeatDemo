/*
 * Copyright 2013 The Android Open Source Project
 * Modified for Cilk heat-diffusion demo by Tao B. Schardl, December 2024
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

#include "gles3jni.h"
#include <jni.h>
#include <cstdlib>
#include <string>
#include <ctime>

// Modelview matrix (Scaling and identity)
const float modelview[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, -1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};
const Vertex QUAD[4] = {
    // Vertices and texture coordinates
    // Position data                   // Texture coordinates
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},  // Bottom-left
    {{-1.0f, 1.0f},  {0.0f, 1.0f}},  // Top-left
    {{1.0f,  1.0f},  {1.0f, 1.0f}},  // Top-right
    {{1.0f,  -1.0f}, {1.0f, 0.0f}},  // Bottom-right
};
const GLuint indices[6] = {0, 1, 2, 0, 2, 3};

bool checkGlError(const char *funcName) {
  GLint err = glGetError();
  if (err != GL_NO_ERROR) {
    ALOGE("GL error after %s(): 0x%08x\n", funcName, err);
    return true;
  }
  return false;
}

GLuint createShader(GLenum shaderType, const char *src) {
  GLuint shader = glCreateShader(shaderType);
  if (!shader) {
    checkGlError("glCreateShader");
    return 0;
  }
  glShaderSource(shader, 1, &src, nullptr);

  GLint compiled = GL_FALSE;
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint infoLogLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
    if (infoLogLen > 0) {
      GLchar *infoLog = (GLchar *) malloc(infoLogLen);
      if (infoLog) {
        glGetShaderInfoLog(shader, infoLogLen, nullptr, infoLog);
        ALOGE("Could not compile %s shader:\n%s\n",
              shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment", infoLog);
        free(infoLog);
      }
    }
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

GLuint createProgram(const char *vtxSrc, const char *fragSrc) {
  GLuint vtxShader = 0;
  GLuint fragShader = 0;
  GLuint program = 0;
  GLint linked = GL_FALSE;

  vtxShader = createShader(GL_VERTEX_SHADER, vtxSrc);
  if (!vtxShader) goto exit;

  fragShader = createShader(GL_FRAGMENT_SHADER, fragSrc);
  if (!fragShader) goto exit;

  program = glCreateProgram();
  if (!program) {
    checkGlError("glCreateProgram");
    goto exit;
  }
  glAttachShader(program, vtxShader);
  glAttachShader(program, fragShader);

  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (!linked) {
    ALOGE("Could not link program");
    GLint infoLogLen = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
    if (infoLogLen) {
      GLchar *infoLog = (GLchar *) malloc(infoLogLen);
      if (infoLog) {
        glGetProgramInfoLog(program, infoLogLen, nullptr, infoLog);
        ALOGE("Could not link program:\n%s\n", infoLog);
        free(infoLog);
      }
    }
    glDeleteProgram(program);
    program = 0;
  }

  exit:
  glDeleteShader(vtxShader);
  glDeleteShader(fragShader);
  return program;
}

static void printGlString(const char *name, GLenum s) {
  const char *v = (const char *) glGetString(s);
  ALOGV("GL %s: %s\n", name, v);
}

// ----------------------------------------------------------------------------

Renderer::Renderer() : mProgram(0), texName(0), mVBState(0), mLastFrameNs(0) {}

Renderer::~Renderer() = default;

void Renderer::resize(int w, int h) {
  calcSceneParams(w, h);
  mLastFrameNs = 0;
  glViewport(0, 0, w, h);
}

void Renderer::calcSceneParams(unsigned int w, unsigned int h) {
  int winW = w;
  int winH = h;
  // Projection from window to grid.
  int rx = w / MUL;
  int ry = h / MUL;
  // Clear old simulation state.
  delete Q;
  if (texImage) {
    free(texImage);
  }
  // Set up simulation state.
  Q = new SimState(rx, ry, true);
  Q->set_sim_size(rx, ry, DEFAULT_TSTEP);
  // Compute X and Y scaling.
  Xscale = float(Q->X) / winW;
  Yscale = float(Q->Y) / winH;
  texImage = (GLubyte *) calloc(GridSize(Q->Xsep, Q->Ysep) * 4, sizeof(GLubyte));
  ALOGV("Q->Xsep %d, Q->Ysep %d, Grid Size %d\n", Q->Xsep, Q->Ysep, GridSize(Q->Xsep, Q->Ysep));
  ALOGV("Xscale %f, Yscale %f\n", Xscale, Yscale);

  glUseProgram(mProgram);
//  // Set up the projection and modelview matrices
//  // Projection matrix (Orthographic)
//  float projection[16] = {
//      2.0f / rx, 0.0f, 0.0f, 0.0f,
//      0.0f, 2.0f / ry, 0.0f, 0.0f,
//      0.0f, 0.0f, -2.0f, 0.0f,
//      -1.0f, 1.0f, 0.0f, 1.0f
//  };

  // Use shaders for rendering
  // Get uniform locations for the projection and modelview matrices
//  GLuint projectionLoc = glGetUniformLocation(mProgram, "u_Projection");
  GLuint modelviewLoc = glGetUniformLocation(mProgram, "u_Modelview");

  // Send the matrices to the shader program
//  glUniformMatrix4fv(projectionLoc, 1, GL_TRUE, projection);
  glUniformMatrix4fv(modelviewLoc, 1, GL_TRUE, modelview);

  glBindTexture(GL_TEXTURE_2D, texName);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // render
  renderTexture();

  ALOGV("x %d, y %d, TexImage() = %x, %x\n", hx, hy, TexImage(Q, hx, hy, 2),
        TexImage(Q, hx, hy, 3));
  glBindTexture(GL_TEXTURE_2D, texName);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Q->Xsep, Q->Ysep, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, texImage);

  ALOGV("calcSceneParams: %d by %d\n", w, h);
}

void Renderer::step() {
  timespec now{};
  clock_gettime(CLOCK_MONOTONIC, &now);
  auto nowNs = now.tv_sec * 1000000000ull + now.tv_nsec;

  // rasterize the hot lines
  recording = 1 - recording;
  {
    int sum = 0;
    short showing = 1 - recording;
    Q->clear_raster_array();
    if (nsegs[showing] > 0) {
      for (int i = 0; i < nsegs[showing] - 1; i++)
        sum += bres(hxs[showing][i], hys[showing][i], hxs[showing][i + 1],
                    hys[showing][i + 1]);
      if (nsegs[showing] == 1) {
        Raster(Q, hys[showing][0], hxs[showing][0]) = hot;
      }
      nsegs[showing] = 0;
      hxs[showing][0] = hx;
      hys[showing][0] = hy;
      sum += 1;
      Q->heat_inc = total_heat_per_frame / sum;
    } else {
      Raster(Q, hy, hx) = hot;
      Q->heat_inc = total_heat_per_frame;
    }
  }

  // TODO: Add logic and UI to select algorithm to run.
//  Q->rect_null(t, t + Q->TStep, 0, Q->X, 0, Q->Y);
  if (mLastFrameNs > 0) {
    int tstep = int(float(nowNs - mLastFrameNs) * REAL_TIME_PER_TSTEP);
    tstep = min(max(1, tstep), DEFAULT_TSTEP);
//    ALOGV("tstep %d\n", tstep);
//    rect_loops_serial(Q, t, t + tstep, 0, Q->X, 0, Q->Y);
//    rect_loops_parallel(Q, t, t + tstep, 0, Q->X, 0, Q->Y);
//    rect_recursive_serial(Q, t, t + tstep, 0, Q->X, 0, Q->Y);
    rect_recursive_dp_ucut(Q, t, t + tstep, 0, Q->X, 0, Q->Y);
    t += tstep;
  }

  // render
  renderTexture();

  glBindTexture(GL_TEXTURE_2D, texName);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Q->Xsep, Q->Ysep, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, texImage);

  mLastFrameNs = nowNs;
}

void Renderer::render() {
  step();

  glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  draw();

  checkGlError("Renderer::render");
}

// ----------------------------------------------------------------------------

static Renderer *g_renderer = nullptr;

#if !defined(DYNAMIC_ES3)

static GLboolean gl3stubInit() { return GL_TRUE; }

#endif

extern "C" {
JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_init([[maybe_unused]] JNIEnv *env,
                                                                       [[maybe_unused]] jclass obj) {
  if (g_renderer) {
    delete g_renderer;
    g_renderer = nullptr;
  }

  printGlString("Version", GL_VERSION);
  printGlString("Vendor", GL_VENDOR);
  printGlString("Renderer", GL_RENDERER);
  printGlString("Extensions", GL_EXTENSIONS);

  const char *versionStr = (const char *) glGetString(GL_VERSION);
  if (strstr(versionStr, "OpenGL ES 3.") && gl3stubInit()) {
    g_renderer = createES3Renderer();
  } else if (strstr(versionStr, "OpenGL ES 2.")) {
    g_renderer = createES2Renderer();
  } else {
    ALOGE("Unsupported OpenGL ES version");
  }

}
JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_resize(
    [[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass obj, jint width, jint height) {
  if (g_renderer) {
    g_renderer->resize(width, height);
  }
}
JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_step([[maybe_unused]] JNIEnv *env,
                                                                       [[maybe_unused]] jclass obj) {
  if (g_renderer) {
    g_renderer->render();
  }
}
JNIEXPORT void JNICALL
Java_com_example_cilkheatdemo2_GLES3JNILib_setXY([[maybe_unused]] JNIEnv *env,
                                                 [[maybe_unused]] jclass obj, jfloat x, jfloat y) {
  if (g_renderer) {
    g_renderer->touchXY(x, y);
  }
}
JNIEXPORT void JNICALL
Java_com_example_cilkheatdemo2_GLES3JNILib_clearXY([[maybe_unused]] JNIEnv *env,
                                                   [[maybe_unused]] jclass obj) {
  if (g_renderer) {
    g_renderer->releaseXY();
  }
}
};
