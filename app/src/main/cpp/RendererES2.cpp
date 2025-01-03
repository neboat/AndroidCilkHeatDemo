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

#include <EGL/egl.h>

#include "gles3jni.h"

// TODO: Rewrite this Renderer implementation to render heat-diffusion simulation.

static const char VERTEX_SHADER[] =
    "#version 100\n"
    "uniform mat2 scaleRot;\n"
    "uniform vec2 offset;\n"
    "attribute vec2 pos;\n"
    "attribute vec4 color;\n"
    "varying vec4 vColor;\n"
    "void main() {\n"
    "    gl_Position = vec4(scaleRot*pos + offset, 0.0, 1.0);\n"
    "    vColor = color;\n"
    "}\n";

static const char FRAGMENT_SHADER[] =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec4 vColor;\n"
    "void main() {\n"
    "    gl_FragColor = vColor;\n"
    "}\n";

class RendererES2 : public Renderer {
 public:
  RendererES2();
  ~RendererES2() override;
  bool init();

 private:
  void draw() override;

  const EGLContext mEglContext;

  GLuint mVB;
  GLint mPosAttrib;
  GLint mTexCoordAttrib;
};

Renderer* createES2Renderer() {
  ALOGE("ES2 Renderer not currently supported.");
  auto* renderer = new RendererES2;
  if (!renderer->init()) {
    delete renderer;
    return nullptr;
  }
  return renderer;
}

RendererES2::RendererES2()
    : mEglContext(eglGetCurrentContext()),
      mVB(0),
      mPosAttrib(-1),
      mTexCoordAttrib(-1) {}

bool RendererES2::init() {
  mProgram = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
  if (!mProgram) return false;
  mPosAttrib = glGetAttribLocation(mProgram, "pos");
  mTexCoordAttrib = glGetAttribLocation(mProgram, "texCoord");

  glGenBuffers(1, &mVB);
  glBindBuffer(GL_ARRAY_BUFFER, mVB);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD), &QUAD[0], GL_STATIC_DRAW);

  ALOGV("Using OpenGL ES 2.0 renderer");
  return true;
}

RendererES2::~RendererES2() {
  /* The destructor may be called after the context has already been
   * destroyed, in which case our objects have already been destroyed.
   *
   * If the context exists, it must be current. This only happens when we're
   * cleaning up after a failed init().
   */
  if (eglGetCurrentContext() != mEglContext) return;
  glDeleteBuffers(1, &mVB);
  glDeleteProgram(mProgram);
}

void RendererES2::draw() {
//  glUseProgram(mProgram);
//
//  glBindBuffer(GL_ARRAY_BUFFER, mVB);
//  glVertexAttribPointer(mPosAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
//                        (const GLvoid*)offsetof(Vertex, pos));
//  glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE,
//                        sizeof(Vertex), (const GLvoid*)offsetof(Vertex, rgba));
//  glEnableVertexAttribArray(mPosAttrib);
//  glEnableVertexAttribArray(mColorAttrib);
//
//  for (unsigned int i = 0; i < numInstances; i++) {
//    glUniformMatrix2fv(mScaleRotUniform, 1, GL_FALSE, mScaleRot + 4 * i);
//    glUniform2fv(mOffsetUniform, 1, mOffsets + 2 * i);
//    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//  }
}
