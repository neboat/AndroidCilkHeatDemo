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

#include "gles3jni.h"

#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cilk/cilk.h>

//const Vertex QUAD[4] = {
//    // Square with diagonal < 2 so that it fits in a [-1 .. 1]^2 square
//    // regardless of rotation.
//    {{-0.7f, -0.7f}, {0x00, 0xFF, 0x00}},
//    {{0.7f, -0.7f}, {0x00, 0x00, 0xFF}},
//    {{-0.7f, 0.7f}, {0xFF, 0x00, 0x00}},
//    {{0.7f, 0.7f}, {0xFF, 0xFF, 0xFF}},
//};
//const Vertex QUAD[4] = {
//        // Square with diagonal < 2 so that it fits in a [-1 .. 1]^2 square
//        // regardless of rotation.
//        {{-1.0f, -1.0f}, {0x00, 0xFF, 0x00}},
//        {{1.0f, -1.0f}, {0x00, 0x00, 0xFF}},
//        {{-1.0f, 1.0f}, {0xFF, 0x00, 0x00}},
//        {{1.0f, 1.0f}, {0xFF, 0xFF, 0xFF}},
//};
//Vertex QUAD[4];
const Vertex QUAD[4] = {
    // Define the vertices and texture coordinates
    // Position data                            // Texture coordinates
    {{-1.0f, -1.0f},               {0.0f, 0.0f}},  // Bottom-left
    {{-1.0f, 1.0f},        {0.0f, 1.0f}},  // Top-left
    {{1.0f, 1.0f}, {1.0f, 1.0f}},  // Top-right
    {{1.0f, -1.0f},        {1.0f, 0.0f}},  // Bottom-right
};
const GLuint indices[6] = { 0, 1, 2, 0, 2, 3 };

bool checkGlError(const char* funcName) {
  GLint err = glGetError();
  if (err != GL_NO_ERROR) {
    ALOGE("GL error after %s(): 0x%08x\n", funcName, err);
    return true;
  }
  return false;
}

GLuint createShader(GLenum shaderType, const char* src) {
  GLuint shader = glCreateShader(shaderType);
  if (!shader) {
    checkGlError("glCreateShader");
    return 0;
  }
  glShaderSource(shader, 1, &src, NULL);

  GLint compiled = GL_FALSE;
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint infoLogLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
    if (infoLogLen > 0) {
      GLchar* infoLog = (GLchar*)malloc(infoLogLen);
      if (infoLog) {
        glGetShaderInfoLog(shader, infoLogLen, NULL, infoLog);
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

GLuint createProgram(const char* vtxSrc, const char* fragSrc) {
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
      GLchar* infoLog = (GLchar*)malloc(infoLogLen);
      if (infoLog) {
        glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
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

static void printGlString(const char* name, GLenum s) {
  const char* v = (const char*)glGetString(s);
  ALOGV("GL %s: %s\n", name, v);
}

// ----------------------------------------------------------------------------

Renderer::Renderer() : mProgram(0), mVBState(0), EBO(0), mNumInstances(0), mLastFrameNs(0) {
  memset(mScale, 0, sizeof(mScale));
  memset(mAngularVelocity, 0, sizeof(mAngularVelocity));
  memset(mAngles, 0, sizeof(mAngles));
}

Renderer::~Renderer() {}

void Renderer::resize(int w, int h) {
  float *offsets = NULL; // mapOffsetBuf();
  calcSceneParams(w, h, offsets);

//  unmapOffsetBuf();

//  // Auto gives a signed int :-(
//  for (auto i = (unsigned)0; i < mNumInstances; i++) {
//    mAngles[i] = drand48() * TWO_PI;
//    mAngularVelocity[i] = MAX_ROT_SPEED * (2.0 * drand48() - 1.0);
//  }
//
  mLastFrameNs = 0;

//  glViewport(0, 0, w, h);
  glViewport(0, 0, w, h);
}

void Renderer::calcSceneParams(unsigned int w, unsigned int h, float* offsets) {
  winW = w;
  winH = h;
  rx = w / MUL;
  ry = h / MUL;
  if (Q) {
    delete Q;
  }
  if (texImage) {
    free(texImage);
  }
  Q = new SimState(w / MUL, h / MUL, true);
  Q->set_sim_size(w / MUL, h / MUL, DEFAULT_TSTEP);
  Xscale = float(Q->X) / winW;
  Yscale = float(Q->Y) / winH;
  texImage = (GLubyte *)calloc(GridSize(Q->Xsep, Q->Ysep) * 4, sizeof(GLubyte));
  ALOGV("Q->Xsep %d, Q->Ysep %d, Grid Size %d\n", Q->Xsep, Q->Ysep, GridSize(Q->Xsep, Q->Ysep));
  ALOGV("Xscale %f, Yscale %f\n", Xscale, Yscale);

  glUseProgram(mProgram);
  glBindTexture(GL_TEXTURE_2D, texName);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // render
  cilk_for (int x = 0; x < Q->X; x++) {
    cilk_for (int y = 0; y < Q->Y; y++) {
      TexImage(Q, x, y, 0) = 0xFF * (U(Q, 0, x, y));
      TexImage(Q, x, y, 1) = 0xFF * (0.5 * U(Q, 0, x, y));
      TexImage(Q, x, y, 2) = 0xFF * (1 - 0.8 * U(Q, 0, x, y));
      TexImage(Q, x, y, 3) = 1;
    }
  }

  ALOGV("x %d, y %d, TexImage() = %x, %x\n", hx, hy, TexImage(Q, hx, hy, 2), TexImage(Q, hx, hy, 3));
  glBindTexture(GL_TEXTURE_2D, texName);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Q->Xsep, Q->Ysep, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, texImage);
//  // Bind the texture in the fragment shader using a sampler2D
//  GLuint texUniform = glGetUniformLocation(mProgram, "u_Texture");
//  glUniform1i(texUniform, 0);  // Set texture unit 0 for the shader

//  // Define the vertices and texture coordinates
//             // Position data                            // Texture coordinates
//  QUAD[0] = {{0.0f, 0.0f},               {0.0f, 0.0f}};  // Bottom-left
//  QUAD[1] = {{0.0f, 0.5f},        {0.0f, 1.0f}};  // Top-left
//  QUAD[2] = {{0.5f, 0.5f}, {1.0f, 1.0f}};  // Top-right
//  QUAD[3] = {{0.5f, 0.0f},        {1.0f, 0.0f}};  // Bottom-right

//  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_INSTANCE]);
//  glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD), &QUAD[0], GL_STATIC_DRAW);
//  GLuint vao, vbo, texCoordVBO;
//  glGenVertexArrays(1, &vao);
//  glBindVertexArray(vao);
//
//  glGenBuffers(1, &vbo);
//  glBindBuffer(GL_ARRAY_BUFFER, vbo);

//  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//
//  // Set vertex attribute pointers
//  GLuint posAttrib = glGetAttribLocation(mProgram, "a_Position");
//  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
//  glEnableVertexAttribArray(posAttrib);
//
//  GLuint texAttrib = glGetAttribLocation(mProgram, "a_TexCoord");
//  glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
//  glEnableVertexAttribArray(texAttrib);

//  // number of cells along the larger screen dimension
//  const float NCELLS_MAJOR = MAX_INSTANCES_PER_SIDE;
//  // cell size in scene space
//  const float CELL_SIZE = 2.0f / NCELLS_MAJOR;
//
//  // Calculations are done in "landscape", i.e. assuming dim[0] >= dim[1].
//  // Only at the end are values put in the opposite order if h > w.
//  const float dim[2] = {fmaxf(w, h), fminf(w, h)};
//  const float aspect[2] = {dim[0] / dim[1], dim[1] / dim[0]};
//  const float scene2clip[2] = {1.0f, aspect[0]};
//  const int ncells[2] = {static_cast<int>(NCELLS_MAJOR),
//                         (int)floorf(NCELLS_MAJOR * aspect[1])};
//
//  float centers[2][MAX_INSTANCES_PER_SIDE];
//  for (int d = 0; d < 2; d++) {
//    auto offset = -ncells[d] / NCELLS_MAJOR;  // -1.0 for d=0
//    for (auto i = 0; i < ncells[d]; i++) {
//      centers[d][i] = scene2clip[d] * (CELL_SIZE * (i + 0.5f) + offset);
//    }
//  }
//
//  int major = w >= h ? 0 : 1;
//  int minor = w >= h ? 1 : 0;
//  // outer product of centers[0] and centers[1]
//  for (int i = 0; i < ncells[0]; i++) {
//    for (int j = 0; j < ncells[1]; j++) {
//      int idx = i * ncells[1] + j;
//      offsets[2 * idx + major] = centers[0][i];
//      offsets[2 * idx + minor] = centers[1][j];
//    }
//  }
//
//  mNumInstances = ncells[0] * ncells[1];
//  mScale[major] = 0.5f * CELL_SIZE * scene2clip[0];
//  mScale[minor] = 0.5f * CELL_SIZE * scene2clip[1];
  ALOGV("calcSceneParams: %d by %d\n", w, h);
}

void Renderer::step() {
  timespec now;
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
  int GridX = Q->X, GridY = Q->Y;
  cilk_for (int x = 0; x < GridX; x++) {
    cilk_for (int y = 0; y < GridY; y++) {
//      ALOGV("x %d, y %d, TexImage() = %p..%p\n", x, y, &TexImage(Q, y, x, 0), &TexImage(Q, y, x, 3));
      TexImage(Q, x, y, 0) = 0xFF * U(Q, 0, x, y);
      TexImage(Q, x, y, 1) = 0xFF * (0.5 * U(Q, 0, x, y));
      TexImage(Q, x, y, 2) = 0xFF * (1 - 0.8 * U(Q, 0, x, y));
      TexImage(Q, x, y, 3) = 1;
    }
  }
//  ALOGV("TexImage[0,0] = %f, %f, %f, %f\n",
//        TexImage(Q, 0, 0, 0), TexImage(Q, 0, 0, 1),
//        TexImage(Q, 0, 0, 2), TexImage(Q, 0, 0, 3));
//  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glBindTexture(GL_TEXTURE_2D, texName);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Q->Xsep, Q->Ysep, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, texImage);

//  glBindTexture(GL_TEXTURE_2D, texName);
//  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Q->Xsep, Q->Ysep, 0, GL_RGBA,
//               GL_FLOAT, texImage);

//  if (mLastFrameNs > 0) {
//    float dt = float(nowNs - mLastFrameNs) * 0.000000001f;
//    float* transforms = mapTransformBuf();
//
//    cilk_for(unsigned int i = 0; i < mNumInstances; i++) {
//        mAngles[i] += mAngularVelocity[i] * dt;
//        if (mAngles[i] >= TWO_PI) {
//            mAngles[i] -= TWO_PI;
//        } else if (mAngles[i] <= -TWO_PI) {
//            mAngles[i] += TWO_PI;
//        }
//    //  }
//    //  cilk_for (unsigned int i = 0; i < mNumInstances; i++) {
//        float s = sinf(mAngles[i]);
//        float c = cosf(mAngles[i]);
//        transforms[4 * i + 0] = c * mScale[0];
//        transforms[4 * i + 1] = s * mScale[1];
//        transforms[4 * i + 2] = -s * mScale[0];
//        transforms[4 * i + 3] = c * mScale[1];
//    }
//    unmapTransformBuf();
//  }

  mLastFrameNs = nowNs;
}

void Renderer::render() {
  step();

  glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // Set up the projection and modelview matrices
  // Projection matrix (Orthographic)
  float projection[16] = {
      2.0f / rx, 0.0f, 0.0f, 0.0f,
      0.0f, 2.0f / ry, 0.0f, 0.0f,
      0.0f, 0.0f, -2.0f, 0.0f,
      -1.0f, 1.0f, 0.0f, 1.0f
  };
  // Modelview matrix (Scaling and identity)
  float modelview[16] = {
      MUL, 0.0f, 0.0f, 0.0f,
      0.0f, -MUL, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
  };

  // Use shaders for rendering
  // Get uniform locations for the projection and modelview matrices
  GLuint projectionLoc = glGetUniformLocation(mProgram, "u_Projection");
  GLuint modelviewLoc = glGetUniformLocation(mProgram, "u_Modelview");

  // Send the matrices to the shader program
  glUniformMatrix4fv(projectionLoc, 1, GL_TRUE, projection);
  glUniformMatrix4fv(modelviewLoc, 1, GL_TRUE, modelview);

  draw(mNumInstances);

  checkGlError("Renderer::render");
}

// ----------------------------------------------------------------------------

static Renderer* g_renderer = nullptr;

// Global sim state variables
//SimArgs theArgs;
//static SimState* g_Q = nullptr;
//long t = 0;  // Global time for simulation.

extern "C" {
JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_init(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_resize(
    JNIEnv* env, jobject obj, jint width, jint height);
JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_step(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_setXY(JNIEnv* env, jobject obj, jfloat x, jfloat y);
JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_clearXY(JNIEnv* env, jobject obj);
};

#if !defined(DYNAMIC_ES3)
static GLboolean gl3stubInit() { return GL_TRUE; }
#endif

JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_init(JNIEnv* env, jobject obj) {
  if (g_renderer) {
    delete g_renderer;
    g_renderer = NULL;
  }

  printGlString("Version", GL_VERSION);
  printGlString("Vendor", GL_VENDOR);
  printGlString("Renderer", GL_RENDERER);
  printGlString("Extensions", GL_EXTENSIONS);

  const char* versionStr = (const char*)glGetString(GL_VERSION);
  if (strstr(versionStr, "OpenGL ES 3.") && gl3stubInit()) {
    g_renderer = createES3Renderer();
  } else if (strstr(versionStr, "OpenGL ES 2.")) {
    g_renderer = createES2Renderer();
  } else {
    ALOGE("Unsupported OpenGL ES version");
  }
}

JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_resize(
    JNIEnv* env, jobject obj, jint width, jint height) {
  if (g_renderer) {
    g_renderer->resize(width, height);
  }
}

JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_step(JNIEnv* env,
                                                                       jobject obj) {
  if (g_renderer) {
    g_renderer->render();
  }
}

JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_setXY(JNIEnv* env,
                                                                        jobject obj,
                                                                        jfloat x, jfloat y) {
  if (g_renderer) {
    g_renderer->touchXY(x, y);
  }
}

JNIEXPORT void JNICALL Java_com_example_cilkheatdemo2_GLES3JNILib_clearXY(JNIEnv* env,
                                                                          jobject obj) {
  if (g_renderer) {
    g_renderer->releaseXY();
  }
}