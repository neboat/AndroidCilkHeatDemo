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
#include "common.h"

#define STR(s) #s
#define STRV(s) STR(s)

#define POS_ATTRIB 0
#define COLOR_ATTRIB 1
#define TEXCOORD_ATTRIB 2
//#define SCALEROT_ATTRIB 2
//#define OFFSET_ATTRIB 3

//static const char VERTEX_SHADER[] =
//    "#version 300 es\n"
//    "layout(location = " STRV(POS_ATTRIB) ") in vec2 pos;\n"
//    "layout(location=" STRV(COLOR_ATTRIB) ") in vec4 color;\n"
//    "layout(location=" STRV(SCALEROT_ATTRIB) ") in vec4 scaleRot;\n"
//    "layout(location=" STRV(OFFSET_ATTRIB) ") in vec2 offset;\n"
//    "out vec4 vColor;\n"
//    "void main() {\n"
//    "    mat2 sr = mat2(scaleRot.xy, scaleRot.zw);\n"
//    "    gl_Position = vec4(sr*pos + offset, 0.0, 1.0);\n"
//    "    vColor = color;\n"
//    "}\n";

//static const char FRAGMENT_SHADER[] =
//        "#version 300 es\n"
//        "precision mediump float;\n"
//        "in vec4 vColor;\n"
//        "out vec4 outColor;\n"
//        "void main() {\n"
//        "    outColor = vColor;\n"
//        "}\n";

static const char VERTEX_SHADER[] =
        "#version 300 es\n"
        "layout(location = " STRV(POS_ATTRIB) ") in vec2 pos;\n"
        "layout(location=" STRV(COLOR_ATTRIB) ") in vec4 color;\n"
        "layout(location=" STRV(TEXCOORD_ATTRIB) ") in vec2 texCoord;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "    gl_Position = vec4(pos, 0.0, 1.0);\n"
        "    TexCoord = texCoord;\n"
        "}\n";

static const char FRAGMENT_SHADER[] =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D ourTexture;\n"
        "void main() {\n"
        "    FragColor = texture(ourTexture, TexCoord);\n"
        "}\n";

class RendererES3 : public Renderer {
 public:
  RendererES3();
  virtual ~RendererES3();
  bool init();

 private:

  virtual float* mapOffsetBuf();
  virtual void unmapOffsetBuf();
  virtual float* mapTransformBuf();
  virtual void unmapTransformBuf();
  virtual void draw(unsigned int numInstances);

  const EGLContext mEglContext;
//  GLuint mProgram;
//  GLuint mVB[VB_COUNT];
//  GLuint mVBState;
};

Renderer* createES3Renderer() {
  RendererES3* renderer = new RendererES3;
  if (!renderer->init()) {
    delete renderer;
    return NULL;
  }
  return renderer;
}

RendererES3::RendererES3()
    : mEglContext(eglGetCurrentContext()) {
  for (int i = 0; i < VB_COUNT; i++) mVB[i] = 0;
}

bool RendererES3::init() {
  mProgram = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
  if (!mProgram) return false;

  glGenBuffers(1, &EBO);
  glGenBuffers(VB_COUNT, mVB);
  glGenTextures(1, &texName);
//  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_INSTANCE]);
//  glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD), &QUAD[0], GL_STATIC_DRAW);
//  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_SCALEROT]);
//  glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * 4 * sizeof(float), NULL,
//               GL_DYNAMIC_DRAW);
//  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_OFFSET]);
//  glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * 2 * sizeof(float), NULL,
//               GL_STATIC_DRAW);
//
  glGenVertexArrays(1, &mVBState);
  glBindVertexArray(mVBState);
//  checkGlError("Renderer::init bind VAO");
  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_INSTANCE]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD), &QUAD[0], GL_STATIC_DRAW);
//  checkGlError("Renderer::init bind VBO");
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
//  checkGlError("Renderer::init bind EBO");
  glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const GLvoid*)offsetof(Vertex, pos));
//  glVertexAttribPointer(COLOR_ATTRIB, 4, GL_UNSIGNED_BYTE, GL_TRUE,
//                        sizeof(Vertex), (const GLvoid*)offsetof(Vertex, rgba));
  glEnableVertexAttribArray(POS_ATTRIB);
  glVertexAttribPointer(TEXCOORD_ATTRIB, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const GLvoid*)offsetof(Vertex, texCoord));
  glEnableVertexAttribArray(TEXCOORD_ATTRIB);
//  glEnableVertexAttribArray(COLOR_ATTRIB);
//
//  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_SCALEROT]);
//  glVertexAttribPointer(SCALEROT_ATTRIB, 4, GL_FLOAT, GL_FALSE,
//                        4 * sizeof(float), 0);
//  glEnableVertexAttribArray(SCALEROT_ATTRIB);
//  glVertexAttribDivisor(SCALEROT_ATTRIB, 1);
//
//  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_OFFSET]);
//  glVertexAttribPointer(OFFSET_ATTRIB, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
//                        0);
//  glEnableVertexAttribArray(OFFSET_ATTRIB);
//  glVertexAttribDivisor(OFFSET_ATTRIB, 1);

//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  ALOGV("Using OpenGL ES 3.0 renderer");
  return true;
}

RendererES3::~RendererES3() {
  /* The destructor may be called after the context has already been
   * destroyed, in which case our objects have already been destroyed.
   *
   * If the context exists, it must be current. This only happens when we're
   * cleaning up after a failed init().
   */
  if (eglGetCurrentContext() != mEglContext) return;
  glDeleteVertexArrays(1, &mVBState);
  glDeleteBuffers(VB_COUNT, mVB);
  glDeleteVertexArrays(1, &EBO);
  glDeleteTextures(1, &texName);
  glDeleteProgram(mProgram);
}

float* RendererES3::mapOffsetBuf() {
//  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_OFFSET]);
//  return (float*)glMapBufferRange(
//      GL_ARRAY_BUFFER, 0, MAX_INSTANCES * 2 * sizeof(float),
//      GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
  return NULL;
}

void RendererES3::unmapOffsetBuf() { glUnmapBuffer(GL_ARRAY_BUFFER); }

float* RendererES3::mapTransformBuf() {
//  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_SCALEROT]);
//  return (float*)glMapBufferRange(
//      GL_ARRAY_BUFFER, 0, MAX_INSTANCES * 4 * sizeof(float),
//      GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
  return NULL;
}

void RendererES3::unmapTransformBuf() { glUnmapBuffer(GL_ARRAY_BUFFER); }

void RendererES3::draw(unsigned int numInstances) {
  glUseProgram(mProgram);
  glBindTexture(GL_TEXTURE_2D, texName);
  glBindVertexArray(mVBState);
//  glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numInstances);

  // Draw the quad
//  glDrawArrays(GL_QUADS, 0, 4);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);

//  // Unbind VAO and cleanup
//  glBindVertexArray(0);
//  glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//  // Disable texture (if you need to)
//  glBindTexture(GL_TEXTURE_2D, 0);

  // Swap buffers (assuming you're using an OpenGL context)
//    eglSwapBuffers(display, surface);  // For EGL context, or use your context's swap buffers function
}
