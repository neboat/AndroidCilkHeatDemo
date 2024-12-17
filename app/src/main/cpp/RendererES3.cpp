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

#include <EGL/egl.h>

#include "gles3jni.h"
#include "common.h"

#define STR(s) #s
#define STRV(s) STR(s)

#define POS_ATTRIB 0
#define TEXCOORD_ATTRIB 1

static const char VERTEX_SHADER[] =
        "#version 300 es\n"
        "layout(location = " STRV(POS_ATTRIB) ") in vec2 pos;\n"
        "layout(location = " STRV(TEXCOORD_ATTRIB) ") in vec2 texCoord;\n"
        "uniform mat4 u_Modelview;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "    gl_Position = u_Modelview * vec4(pos, 0.0, 1.0);\n"
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
  ~RendererES3() override;
  bool init();

 private:
  void draw() override;

  const EGLContext mEglContext;
  GLuint EBO;
};

Renderer* createES3Renderer() {
  auto* renderer = new RendererES3;
  if (!renderer->init()) {
    delete renderer;
    return nullptr;
  }
  return renderer;
}

RendererES3::RendererES3()
    : mEglContext(eglGetCurrentContext()), EBO(0) {
  for (unsigned int &i : mVB) i = 0;
}

bool RendererES3::init() {
  mProgram = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
  if (!mProgram) return false;

  glGenBuffers(1, &EBO);
  glGenBuffers(VB_COUNT, mVB);
  glGenTextures(1, &texName);

  glGenVertexArrays(1, &mVBState);
  glBindVertexArray(mVBState);

  glBindBuffer(GL_ARRAY_BUFFER, mVB[VB_INSTANCE]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD), &QUAD[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const GLvoid*)offsetof(Vertex, pos));

  glEnableVertexAttribArray(POS_ATTRIB);
  glVertexAttribPointer(TEXCOORD_ATTRIB, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (const GLvoid*)offsetof(Vertex, texCoord));
  glEnableVertexAttribArray(TEXCOORD_ATTRIB);

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

void RendererES3::draw() {
  glUseProgram(mProgram);
  glBindTexture(GL_TEXTURE_2D, texName);
  glBindVertexArray(mVBState);

  // Draw the quad
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}
