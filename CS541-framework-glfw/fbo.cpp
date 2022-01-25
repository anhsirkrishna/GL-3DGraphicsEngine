///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a Frame Buffer Object (i'e' Render
// Target) and its associated texture.  When the FBO is "Bound", the
// output of the graphics pipeline is captured into the texture.  When
// it is "Unbound", the texture is available for use as any normal
// texture.
////////////////////////////////////////////////////////////////////////

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#include "fbo.h"

void FBO::CreateFBO(const int w, const int h, const int _color_attachment_count)
{
    width = w;
    height = h;
    color_attachment_count = _color_attachment_count;
    glGenFramebuffersEXT(1, &fboID);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);

    // Create a render buffer, and attach it to FBO's depth attachment
    unsigned int depthBuffer;
    glGenRenderbuffersEXT(1, &depthBuffer);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthBuffer);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
                             width, height);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, depthBuffer);

    // Create a texture and attach FBO's color 0 attachment.  The
    // GL_RGBA32F and GL_RGBA constants set this texture to be 32 bit
    // floats for each of the 4 components.  Many other choices are
    // possible.
    for (int i = 0; i < color_attachment_count; i++) {
        glGenTextures(1, &textureID[i]);
        glBindTexture(GL_TEXTURE_2D, textureID[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, (int)GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (int)GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (int)GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)GL_LINEAR);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, (gl::GLenum)((int)GL_COLOR_ATTACHMENT0_EXT + i),
            GL_TEXTURE_2D, textureID[i], 0);
    }
    // Check for completeness/correctness
    int status = (int)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status != int(GL_FRAMEBUFFER_COMPLETE_EXT))
        printf("FBO Error: %d\n", status);

    // Unbind the fbo until it's ready to be used
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


void FBO::Bind() { glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID); }
void FBO::Unbind() { glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); }

void FBO::BindTexture(const int program_id, const int texture_unit, const char* var_name, const int color_attachment) {
    glActiveTexture((gl::GLenum)(int)GL_TEXTURE0 + texture_unit); 
    glBindTexture(GL_TEXTURE_2D, textureID[color_attachment]); // Load texture into it
    int loc = glGetUniformLocation(program_id, var_name);
    glUniform1i(loc, texture_unit);
}

void FBO::UnbindTexture(const int texture_unit) {
    glActiveTexture((gl::GLenum)(int)GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, 0); // Load texture into it
}

void FBO::Resize(const int w, const int h) {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);
    for (int i = 0; i < color_attachment_count; i++) {
        glBindTexture(GL_TEXTURE_2D, textureID[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, (int)GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, (gl::GLenum)((int)GL_COLOR_ATTACHMENT0_EXT + i),
            GL_TEXTURE_2D, textureID[i], 0);
    }
    int status = (int)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status != int(GL_FRAMEBUFFER_COMPLETE_EXT))
        printf("FBO Error: %d\n", status);
    glBindTexture(GL_TEXTURE_2D, 0);
}