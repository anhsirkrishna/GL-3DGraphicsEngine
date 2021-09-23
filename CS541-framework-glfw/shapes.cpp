////////////////////////////////////////////////////////////////////////
// A small library of object shapes (ground plane, sphere, and the
// famous Utah teapot), each created as a Vertex Array Object (VAO).
// This is the most efficient way to get geometry into the OpenGL
// graphics pipeline.
//
// Each vertex is specified as four attributes which are made
// available in a vertex shader in the following attribute slots.
//
// position,        vec4,   attribute #0
// normal,          vec3,   attribute #1
// texture coord,   vec3,   attribute #2
// tangent,         vec3,   attribute #3
//
// An instance of any of these shapes is create with a single call:
//    unsigned int obj = CreateSphere(divisions, &quadCount);
// and drawn by:
//    glBindVertexArray(vaoID);
//    glDrawElements(GL_TRIANGLES, vertexcount, GL_UNSIGNED_INT, 0);
//    glBindVertexArray(0);
////////////////////////////////////////////////////////////////////////

#include <vector>
#include <fstream>
#include <stdlib.h>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#include <glu.h>                // For gluErrorString
#define CHECKERROR {GLenum err = glGetError(); if (err != GL_NO_ERROR) { fprintf(stderr, "OpenGL error (at line shapes.cpp:%d): %s\n", __LINE__, gluErrorString(err)); exit(-1);} }

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "math.h"
#include "shapes.h"
#include "rply.h"
#include "simplexnoise.h"

const float PI = 3.14159f;
const float rad = PI/180.0f;

void pushquad(std::vector<glm::ivec3> &Tri, int i, int j, int k, int l)
{
    Tri.push_back(glm::ivec3(i,j,k));
    Tri.push_back(glm::ivec3(i,k,l));
}

// Batch up all the data defining a shape to be drawn (example: the
// teapot) as a Vertex Array object (VAO) and send it to the graphics
// card.  Return an OpenGL identifier for the created VAO.
unsigned int VaoFromTris(std::vector<glm::vec4> Pnt,
                         std::vector<glm::vec3> Nrm,
                         std::vector<glm::vec2> Tex,
                         std::vector<glm::vec3> Tan,
                         std::vector<glm::ivec3> Tri)
{
    printf("VaoFromTris %ld %ld\n", Pnt.size(), Tri.size());
    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    GLuint Pbuff;
    glGenBuffers(1, &Pbuff);
    glBindBuffer(GL_ARRAY_BUFFER, Pbuff);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*Pnt.size(),
                 &Pnt[0][0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (Nrm.size() > 0) {
        GLuint Nbuff;
        glGenBuffers(1, &Nbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Nbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*Nrm.size(),
                     &Nrm[0][0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); }

    if (Tex.size() > 0) {
        GLuint Tbuff;
        glGenBuffers(1, &Tbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Tbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*Tex.size(),
                     &Tex[0][0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); }

    if (Tan.size() > 0) {
        GLuint Dbuff;
        glGenBuffers(1, &Dbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Dbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*Tan.size(),
                     &Tan[0][0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); }

    GLuint Ibuff;
    glGenBuffers(1, &Ibuff);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Ibuff);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*3*Tri.size(),
                 &Tri[0][0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    return vaoID;
}

void Shape::ComputeSize()
{
    // Compute min/max
    minP = (Pnt[0]).xyz();
    maxP = (Pnt[0]).xyz();
    for (std::vector<glm::vec4>::iterator p=Pnt.begin();  p<Pnt.end();  p++)
        for (int c=0;  c<3;  c++) {
            minP[c] = std::min(minP[c], (*p)[c]);
            maxP[c] = std::max(maxP[c], (*p)[c]); }
    
    center = (maxP+minP)/2.0f;
    size = 0.0;
    for (int c=0;  c<3;  c++)
        size = std::max(size, (maxP[c]-minP[c])/2.0f);

    float s = 1.0/size;
    modelTr = Scale(s,s,s)*Translate(-center[0], -center[1], -center[2]);
}

void Shape::MakeVAO()
{
    vaoID = VaoFromTris(Pnt, Nrm, Tex, Tan, Tri);
    count = Tri.size();
}

void Shape::DrawVAO()
{
    CHECKERROR;
    glBindVertexArray(vaoID);
    CHECKERROR;
    glDrawElements(GL_TRIANGLES, 3*count, GL_UNSIGNED_INT, 0);
    CHECKERROR;
    glBindVertexArray(0);
}

////////////////////////////////////////////////////////////////////////////////
// Data for the Utah teapot.  It consists of a list of 306 control
// points, and 32 Bezier patches, each defined by 16 control points
// (specified as 1-based indices into the control point array).  This
// evaluates the Bernstein basis functions directly to compute the
// vertices.  This is not the most efficient, but it is the
// easiest. (Which is fine for an operation done once at startup
// time.)
unsigned int TeapotIndex[][16] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
    4, 17, 18, 19,  8, 20, 21, 22, 12, 23, 24, 25, 16, 26, 27, 28,
    19, 29, 30, 31, 22, 32, 33, 34, 25, 35, 36, 37, 28, 38, 39, 40,
    31, 41, 42,  1, 34, 43, 44,  5, 37, 45, 46,  9, 40, 47, 48, 13,
    13, 14, 15, 16, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    16, 26, 27, 28, 52, 61, 62, 63, 56, 64, 65, 66, 60, 67, 68, 69,
    28, 38, 39, 40, 63, 70, 71, 72, 66, 73, 74, 75, 69, 76, 77, 78,
    40, 47, 48, 13, 72, 79, 80, 49, 75, 81, 82, 53, 78, 83, 84, 57,
    57, 58, 59, 60, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
    60, 67, 68, 69, 88, 97, 98, 99, 92,100,101,102, 96,103,104,105,
    69, 76, 77, 78, 99,106,107,108,102,109,110,111,105,112,113,114,
    78, 83, 84, 57,108,115,116, 85,111,117,118, 89,114,119,120, 93,
    121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,
    124,137,138,121,128,139,140,125,132,141,142,129,136,143,144,133,
    133,134,135,136,145,146,147,148,149,150,151,152, 69,153,154,155,
    136,143,144,133,148,156,157,145,152,158,159,149,155,160,161, 69,
    162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,
    165,178,179,162,169,180,181,166,173,182,183,170,177,184,185,174,
    174,175,176,177,186,187,188,189,190,191,192,193,194,195,196,197,
    177,184,185,174,189,198,199,186,193,200,201,190,197,202,203,194,
    204,204,204,204,207,208,209,210,211,211,211,211,212,213,214,215,
    204,204,204,204,210,217,218,219,211,211,211,211,215,220,221,222,
    204,204,204,204,219,224,225,226,211,211,211,211,222,227,228,229,
    204,204,204,204,226,230,231,207,211,211,211,211,229,232,233,212,
    212,213,214,215,234,235,236,237,238,239,240,241,242,243,244,245,
    215,220,221,222,237,246,247,248,241,249,250,251,245,252,253,254,
    222,227,228,229,248,255,256,257,251,258,259,260,254,261,262,263,
    229,232,233,212,257,264,265,234,260,266,267,238,263,268,269,242,
    270,270,270,270,279,280,281,282,275,276,277,278,271,272,273,274,
    270,270,270,270,282,289,290,291,278,286,287,288,274,283,284,285,
    270,270,270,270,291,298,299,300,288,295,296,297,285,292,293,294,
    270,270,270,270,300,305,306,279,297,303,304,275,294,301,302,271};

glm::vec3 TeapotPoints[] = {
    glm::vec3(1.4,0.0,2.4), glm::vec3(1.4,-0.784,2.4), glm::vec3(0.784,-1.4,2.4),
    glm::vec3(0.0,-1.4,2.4), glm::vec3(1.3375,0.0,2.53125),
    glm::vec3(1.3375,-0.749,2.53125), glm::vec3(0.749,-1.3375,2.53125),
    glm::vec3(0.0,-1.3375,2.53125), glm::vec3(1.4375,0.0,2.53125),
    glm::vec3(1.4375,-0.805,2.53125), glm::vec3(0.805,-1.4375,2.53125),
    glm::vec3(0.0,-1.4375,2.53125), glm::vec3(1.5,0.0,2.4), glm::vec3(1.5,-0.84,2.4),
    glm::vec3(0.84,-1.5,2.4), glm::vec3(0.0,-1.5,2.4), glm::vec3(-0.784,-1.4,2.4),
    glm::vec3(-1.4,-0.784,2.4), glm::vec3(-1.4,0.0,2.4),
    glm::vec3(-0.749,-1.3375,2.53125), glm::vec3(-1.3375,-0.749,2.53125),
    glm::vec3(-1.3375,0.0,2.53125), glm::vec3(-0.805,-1.4375,2.53125),
    glm::vec3(-1.4375,-0.805,2.53125), glm::vec3(-1.4375,0.0,2.53125),
    glm::vec3(-0.84,-1.5,2.4), glm::vec3(-1.5,-0.84,2.4), glm::vec3(-1.5,0.0,2.4),
    glm::vec3(-1.4,0.784,2.4), glm::vec3(-0.784,1.4,2.4), glm::vec3(0.0,1.4,2.4),
    glm::vec3(-1.3375,0.749,2.53125), glm::vec3(-0.749,1.3375,2.53125),
    glm::vec3(0.0,1.3375,2.53125), glm::vec3(-1.4375,0.805,2.53125),
    glm::vec3(-0.805,1.4375,2.53125), glm::vec3(0.0,1.4375,2.53125),
    glm::vec3(-1.5,0.84,2.4), glm::vec3(-0.84,1.5,2.4), glm::vec3(0.0,1.5,2.4),
    glm::vec3(0.784,1.4,2.4), glm::vec3(1.4,0.784,2.4), glm::vec3(0.749,1.3375,2.53125),
    glm::vec3(1.3375,0.749,2.53125), glm::vec3(0.805,1.4375,2.53125),
    glm::vec3(1.4375,0.805,2.53125), glm::vec3(0.84,1.5,2.4), glm::vec3(1.5,0.84,2.4),
    glm::vec3(1.75,0.0,1.875), glm::vec3(1.75,-0.98,1.875), glm::vec3(0.98,-1.75,1.875),
    glm::vec3(0.0,-1.75,1.875), glm::vec3(2.0,0.0,1.35), glm::vec3(2.0,-1.12,1.35),
    glm::vec3(1.12,-2.0,1.35), glm::vec3(0.0,-2.0,1.35), glm::vec3(2.0,0.0,0.9),
    glm::vec3(2.0,-1.12,0.9), glm::vec3(1.12,-2.0,0.9), glm::vec3(0.0,-2.0,0.9),
    glm::vec3(-0.98,-1.75,1.875), glm::vec3(-1.75,-0.98,1.875),
    glm::vec3(-1.75,0.0,1.875), glm::vec3(-1.12,-2.0,1.35), glm::vec3(-2.0,-1.12,1.35),
    glm::vec3(-2.0,0.0,1.35), glm::vec3(-1.12,-2.0,0.9), glm::vec3(-2.0,-1.12,0.9),
    glm::vec3(-2.0,0.0,0.9), glm::vec3(-1.75,0.98,1.875), glm::vec3(-0.98,1.75,1.875),
    glm::vec3(0.0,1.75,1.875), glm::vec3(-2.0,1.12,1.35), glm::vec3(-1.12,2.0,1.35),
    glm::vec3(0.0,2.0,1.35), glm::vec3(-2.0,1.12,0.9), glm::vec3(-1.12,2.0,0.9),
    glm::vec3(0.0,2.0,0.9), glm::vec3(0.98,1.75,1.875), glm::vec3(1.75,0.98,1.875),
    glm::vec3(1.12,2.0,1.35), glm::vec3(2.0,1.12,1.35), glm::vec3(1.12,2.0,0.9),
    glm::vec3(2.0,1.12,0.9), glm::vec3(2.0,0.0,0.45), glm::vec3(2.0,-1.12,0.45),
    glm::vec3(1.12,-2.0,0.45), glm::vec3(0.0,-2.0,0.45), glm::vec3(1.5,0.0,0.225),
    glm::vec3(1.5,-0.84,0.225), glm::vec3(0.84,-1.5,0.225), glm::vec3(0.0,-1.5,0.225),
    glm::vec3(1.5,0.0,0.15), glm::vec3(1.5,-0.84,0.15), glm::vec3(0.84,-1.5,0.15),
    glm::vec3(0.0,-1.5,0.15), glm::vec3(-1.12,-2.0,0.45), glm::vec3(-2.0,-1.12,0.45),
    glm::vec3(-2.0,0.0,0.45), glm::vec3(-0.84,-1.5,0.225), glm::vec3(-1.5,-0.84,0.225),
    glm::vec3(-1.5,0.0,0.225), glm::vec3(-0.84,-1.5,0.15), glm::vec3(-1.5,-0.84,0.15),
    glm::vec3(-1.5,0.0,0.15), glm::vec3(-2.0,1.12,0.45), glm::vec3(-1.12,2.0,0.45),
    glm::vec3(0.0,2.0,0.45), glm::vec3(-1.5,0.84,0.225), glm::vec3(-0.84,1.5,0.225),
    glm::vec3(0.0,1.5,0.225), glm::vec3(-1.5,0.84,0.15), glm::vec3(-0.84,1.5,0.15),
    glm::vec3(0.0,1.5,0.15), glm::vec3(1.12,2.0,0.45), glm::vec3(2.0,1.12,0.45),
    glm::vec3(0.84,1.5,0.225), glm::vec3(1.5,0.84,0.225), glm::vec3(0.84,1.5,0.15),
    glm::vec3(1.5,0.84,0.15), glm::vec3(-1.6,0.0,2.025), glm::vec3(-1.6,-0.3,2.025),
    glm::vec3(-1.5,-0.3,2.25), glm::vec3(-1.5,0.0,2.25), glm::vec3(-2.3,0.0,2.025),
    glm::vec3(-2.3,-0.3,2.025), glm::vec3(-2.5,-0.3,2.25), glm::vec3(-2.5,0.0,2.25),
    glm::vec3(-2.7,0.0,2.025), glm::vec3(-2.7,-0.3,2.025), glm::vec3(-3.0,-0.3,2.25),
    glm::vec3(-3.0,0.0,2.25), glm::vec3(-2.7,0.0,1.8), glm::vec3(-2.7,-0.3,1.8),
    glm::vec3(-3.0,-0.3,1.8), glm::vec3(-3.0,0.0,1.8), glm::vec3(-1.5,0.3,2.25),
    glm::vec3(-1.6,0.3,2.025), glm::vec3(-2.5,0.3,2.25), glm::vec3(-2.3,0.3,2.025),
    glm::vec3(-3.0,0.3,2.25), glm::vec3(-2.7,0.3,2.025), glm::vec3(-3.0,0.3,1.8),
    glm::vec3(-2.7,0.3,1.8), glm::vec3(-2.7,0.0,1.575), glm::vec3(-2.7,-0.3,1.575),
    glm::vec3(-3.0,-0.3,1.35), glm::vec3(-3.0,0.0,1.35), glm::vec3(-2.5,0.0,1.125),
    glm::vec3(-2.5,-0.3,1.125), glm::vec3(-2.65,-0.3,0.9375),
    glm::vec3(-2.65,0.0,0.9375), glm::vec3(-2.0,-0.3,0.9), glm::vec3(-1.9,-0.3,0.6),
    glm::vec3(-1.9,0.0,0.6), glm::vec3(-3.0,0.3,1.35), glm::vec3(-2.7,0.3,1.575),
    glm::vec3(-2.65,0.3,0.9375), glm::vec3(-2.5,0.3,1.125), glm::vec3(-1.9,0.3,0.6),
    glm::vec3(-2.0,0.3,0.9), glm::vec3(1.7,0.0,1.425), glm::vec3(1.7,-0.66,1.425),
    glm::vec3(1.7,-0.66,0.6), glm::vec3(1.7,0.0,0.6), glm::vec3(2.6,0.0,1.425),
    glm::vec3(2.6,-0.66,1.425), glm::vec3(3.1,-0.66,0.825), glm::vec3(3.1,0.0,0.825),
    glm::vec3(2.3,0.0,2.1), glm::vec3(2.3,-0.25,2.1), glm::vec3(2.4,-0.25,2.025),
    glm::vec3(2.4,0.0,2.025), glm::vec3(2.7,0.0,2.4), glm::vec3(2.7,-0.25,2.4),
    glm::vec3(3.3,-0.25,2.4), glm::vec3(3.3,0.0,2.4), glm::vec3(1.7,0.66,0.6),
    glm::vec3(1.7,0.66,1.425), glm::vec3(3.1,0.66,0.825), glm::vec3(2.6,0.66,1.425),
    glm::vec3(2.4,0.25,2.025), glm::vec3(2.3,0.25,2.1), glm::vec3(3.3,0.25,2.4),
    glm::vec3(2.7,0.25,2.4), glm::vec3(2.8,0.0,2.475), glm::vec3(2.8,-0.25,2.475),
    glm::vec3(3.525,-0.25,2.49375), glm::vec3(3.525,0.0,2.49375),
    glm::vec3(2.9,0.0,2.475), glm::vec3(2.9,-0.15,2.475), glm::vec3(3.45,-0.15,2.5125),
    glm::vec3(3.45,0.0,2.5125), glm::vec3(2.8,0.0,2.4), glm::vec3(2.8,-0.15,2.4),
    glm::vec3(3.2,-0.15,2.4), glm::vec3(3.2,0.0,2.4), glm::vec3(3.525,0.25,2.49375),
    glm::vec3(2.8,0.25,2.475), glm::vec3(3.45,0.15,2.5125), glm::vec3(2.9,0.15,2.475),
    glm::vec3(3.2,0.15,2.4), glm::vec3(2.8,0.15,2.4), glm::vec3(0.0,0.0,3.15),
    glm::vec3(0.0,-0.002,3.15), glm::vec3(0.002,0.0,3.15), glm::vec3(0.8,0.0,3.15),
    glm::vec3(0.8,-0.45,3.15), glm::vec3(0.45,-0.8,3.15), glm::vec3(0.0,-0.8,3.15),
    glm::vec3(0.0,0.0,2.85), glm::vec3(0.2,0.0,2.7), glm::vec3(0.2,-0.112,2.7),
    glm::vec3(0.112,-0.2,2.7), glm::vec3(0.0,-0.2,2.7), glm::vec3(-0.002,0.0,3.15),
    glm::vec3(-0.45,-0.8,3.15), glm::vec3(-0.8,-0.45,3.15), glm::vec3(-0.8,0.0,3.15),
    glm::vec3(-0.112,-0.2,2.7), glm::vec3(-0.2,-0.112,2.7), glm::vec3(-0.2,0.0,2.7),
    glm::vec3(0.0,0.002,3.15), glm::vec3(-0.8,0.45,3.15), glm::vec3(-0.45,0.8,3.15),
    glm::vec3(0.0,0.8,3.15), glm::vec3(-0.2,0.112,2.7), glm::vec3(-0.112,0.2,2.7),
    glm::vec3(0.0,0.2,2.7), glm::vec3(0.45,0.8,3.15), glm::vec3(0.8,0.45,3.15),
    glm::vec3(0.112,0.2,2.7), glm::vec3(0.2,0.112,2.7), glm::vec3(0.4,0.0,2.55),
    glm::vec3(0.4,-0.224,2.55), glm::vec3(0.224,-0.4,2.55), glm::vec3(0.0,-0.4,2.55),
    glm::vec3(1.3,0.0,2.55), glm::vec3(1.3,-0.728,2.55), glm::vec3(0.728,-1.3,2.55),
    glm::vec3(0.0,-1.3,2.55), glm::vec3(1.3,0.0,2.4), glm::vec3(1.3,-0.728,2.4),
    glm::vec3(0.728,-1.3,2.4), glm::vec3(0.0,-1.3,2.4), glm::vec3(-0.224,-0.4,2.55),
    glm::vec3(-0.4,-0.224,2.55), glm::vec3(-0.4,0.0,2.55), glm::vec3(-0.728,-1.3,2.55),
    glm::vec3(-1.3,-0.728,2.55), glm::vec3(-1.3,0.0,2.55), glm::vec3(-0.728,-1.3,2.4),
    glm::vec3(-1.3,-0.728,2.4), glm::vec3(-1.3,0.0,2.4), glm::vec3(-0.4,0.224,2.55),
    glm::vec3(-0.224,0.4,2.55), glm::vec3(0.0,0.4,2.55), glm::vec3(-1.3,0.728,2.55),
    glm::vec3(-0.728,1.3,2.55), glm::vec3(0.0,1.3,2.55), glm::vec3(-1.3,0.728,2.4),
    glm::vec3(-0.728,1.3,2.4), glm::vec3(0.0,1.3,2.4), glm::vec3(0.224,0.4,2.55),
    glm::vec3(0.4,0.224,2.55), glm::vec3(0.728,1.3,2.55), glm::vec3(1.3,0.728,2.55),
    glm::vec3(0.728,1.3,2.4), glm::vec3(1.3,0.728,2.4), glm::vec3(0.0,0.0,0.0),
    glm::vec3(1.5,0.0,0.15), glm::vec3(1.5,0.84,0.15), glm::vec3(0.84,1.5,0.15),
    glm::vec3(0.0,1.5,0.15), glm::vec3(1.5,0.0,0.075), glm::vec3(1.5,0.84,0.075),
    glm::vec3(0.84,1.5,0.075), glm::vec3(0.0,1.5,0.075), glm::vec3(1.425,0.0,0.0),
    glm::vec3(1.425,0.798,0.0), glm::vec3(0.798,1.425,0.0), glm::vec3(0.0,1.425,0.0),
    glm::vec3(-0.84,1.5,0.15), glm::vec3(-1.5,0.84,0.15), glm::vec3(-1.5,0.0,0.15),
    glm::vec3(-0.84,1.5,0.075), glm::vec3(-1.5,0.84,0.075), glm::vec3(-1.5,0.0,0.075),
    glm::vec3(-0.798,1.425,0.0), glm::vec3(-1.425,0.798,0.0), glm::vec3(-1.425,0.0,0.0),
    glm::vec3(-1.5,-0.84,0.15), glm::vec3(-0.84,-1.5,0.15), glm::vec3(0.0,-1.5,0.15),
    glm::vec3(-1.5,-0.84,0.075), glm::vec3(-0.84,-1.5,0.075), glm::vec3(0.0,-1.5,0.075),
    glm::vec3(-1.425,-0.798,0.0), glm::vec3(-0.798,-1.425,0.0),
    glm::vec3(0.0,-1.425,0.0), glm::vec3(0.84,-1.5,0.15), glm::vec3(1.5,-0.84,0.15),
    glm::vec3(0.84,-1.5,0.075), glm::vec3(1.5,-0.84,0.075), glm::vec3(0.798,-1.425,0.0),
    glm::vec3(1.425,-0.798,0.0)};

////////////////////////////////////////////////////////////////////////////////
// Builds a Vertex Array Object for the Utah teapot.  Each of the 32
// patches is represented by an n by n grid of quads triangulated.
Teapot::Teapot(const int n)
{
    diffuseColor = glm::vec3(0.5, 0.5, 0.1);
    specularColor = glm::vec3(1.0, 1.0, 1.0);
    shininess = 120.0;
    animate = true;

    int npatches = sizeof(TeapotIndex)/sizeof(TeapotIndex[0]); // Should be 32 patches for the teapot
    const int nv = npatches*(n+1)*(n+1);
    int nq = npatches*n*n;

    for (int p=0;  p<npatches;  p++)    { // For each patch
        for (int i=0;  i<=n; i++) {       // Grid in u direction
            float u = float(i)/n;
            for (int j=0;  j<=n; j++) { // Grid if v direction
                float v = float(j)/n;

                // Four u weights
                float u0 = (1.0-u)*(1.0-u)*(1.0-u);
                float u1 = 3.0*(1.0-u)*(1.0-u)*u;
                float u2 = 3.0*(1.0-u)*u*u;
                float u3 = u*u*u;

                // Three du weights
                float du0 = (1.0-u)*(1.0-u);
                float du1 = 2.0*(1.0-u)*u;
                float du2 = u*u;

                // Four v weights
                float v0 = (1.0-v)*(1.0-v)*(1.0-v);
                float v1 = 3.0*(1.0-v)*(1.0-v)*v;
                float v2 = 3.0*(1.0-v)*v*v;
                float v3 = v*v*v;

                // Three dv weights
                float dv0 = (1.0-v)*(1.0-v);
                float dv1 = 2.0*(1.0-v)*v;
                float dv2 = v*v;

                // Grab the 16 control points for Bezier patch.
                glm::vec3* p00 = &TeapotPoints[TeapotIndex[p][ 0]-1];
                glm::vec3* p01 = &TeapotPoints[TeapotIndex[p][ 1]-1];
                glm::vec3* p02 = &TeapotPoints[TeapotIndex[p][ 2]-1];
                glm::vec3* p03 = &TeapotPoints[TeapotIndex[p][ 3]-1];
                glm::vec3* p10 = &TeapotPoints[TeapotIndex[p][ 4]-1];
                glm::vec3* p11 = &TeapotPoints[TeapotIndex[p][ 5]-1];
                glm::vec3* p12 = &TeapotPoints[TeapotIndex[p][ 6]-1];
                glm::vec3* p13 = &TeapotPoints[TeapotIndex[p][ 7]-1];
                glm::vec3* p20 = &TeapotPoints[TeapotIndex[p][ 8]-1];
                glm::vec3* p21 = &TeapotPoints[TeapotIndex[p][ 9]-1];
                glm::vec3* p22 = &TeapotPoints[TeapotIndex[p][10]-1];
                glm::vec3* p23 = &TeapotPoints[TeapotIndex[p][11]-1];
                glm::vec3* p30 = &TeapotPoints[TeapotIndex[p][12]-1];
                glm::vec3* p31 = &TeapotPoints[TeapotIndex[p][13]-1];
                glm::vec3* p32 = &TeapotPoints[TeapotIndex[p][14]-1];
                glm::vec3* p33 = &TeapotPoints[TeapotIndex[p][15]-1];

                // Evaluate the Bezier patch at (u,v)
                glm::vec3 V =
                    u0*v0*(*p00) + u0*v1*(*p01) + u0*v2*(*p02) + u0*v3*(*p03) +
                    u1*v0*(*p10) + u1*v1*(*p11) + u1*v2*(*p12) + u1*v3*(*p13) +
                    u2*v0*(*p20) + u2*v1*(*p21) + u2*v2*(*p22) + u2*v3*(*p23) +
                    u3*v0*(*p30) + u3*v1*(*p31) + u3*v2*(*p32) + u3*v3*(*p33);
                //*pp++ = glm::vec4(V[0], V[1], V[2], 1.0);
                Pnt.push_back(glm::vec4(V[0], V[1], V[2], 1.0));
                Tex.push_back(glm::vec2(u,v));

                // Evaluate the u-tangent of the Bezier patch at (u,v)
                glm::vec3 du =
                    du0*v0*(*p10-*p00) + du0*v1*(*p11-*p01) + du0*v2*(*p12-*p02) + du0*v3*(*p13-*p03) +
                    du1*v0*(*p20-*p10) + du1*v1*(*p21-*p11) + du1*v2*(*p22-*p12) + du1*v3*(*p23-*p13) +
                    du2*v0*(*p30-*p20) + du2*v1*(*p31-*p21) + du2*v2*(*p32-*p22) + du2*v3*(*p33-*p23);
                Tan.push_back(du);

                // Evaluate the v-tangent of the Bezier patch at (u,v)
                glm::vec3 dv =
                    u0*dv0*(*p01-*p00) + u0*dv1*(*p02-*p01) + u0*dv2*(*p03-*p02) +
                    u1*dv0*(*p11-*p10) + u1*dv1*(*p12-*p11) + u1*dv2*(*p13-*p12) +
                    u2*dv0*(*p21-*p20) + u2*dv1*(*p22-*p21) + u2*dv2*(*p23-*p22) +
                    u3*dv0*(*p31-*p30) + u3*dv1*(*p32-*p31) + u3*dv2*(*p33-*p32);

                // Calculate the surface normal as the cross product of the two tangents.
                Nrm.push_back(glm::cross(dv,du));

                //-(du[1]*dv[2]-du[2]*dv[1]);
                //*np++ = -(du[2]*dv[0]-du[0]*dv[2]);
                //*np++ = -(du[0]*dv[1]-du[1]*dv[0]);

                // Create a quad for all but the first edge vertices
                if (i>0 && j>0) 
                    pushquad(Tri,
                             p*(n+1)*(n+1) + (i-1)*(n+1) + (j-1),
                             p*(n+1)*(n+1) + (i-1)*(n+1) + (j),
                             p*(n+1)*(n+1) + (i  )*(n+1) + (j),
                             p*(n+1)*(n+1) + (i  )*(n+1) + (j-1)); } } }
    ComputeSize();
    MakeVAO();
}


////////////////////////////////////////////////////////////////////////
// Generates a box +-1 on all axes
Box::Box()
{
    diffuseColor = glm::vec3(0.5, 0.5, 1.0);
    specularColor = glm::vec3(1.0, 1.0, 1.0);
    shininess = 120.0;

    glm::mat4 I(1.0f);

    // Six faces, each a rotation of a rectangle placed on the z axis.
    face(I);
    float r90 = PI/2;
    face(glm::rotate(I,  r90, glm::vec3(1.0f, 0.0f, 0.0f)));
    face(glm::rotate(I, -r90, glm::vec3(1.0f, 0.0f, 0.0f)));
    face(glm::rotate(I,  r90, glm::vec3(0.0f, 1.0f, 0.0f)));
    face(glm::rotate(I, -r90, glm::vec3(0.0f, 1.0f, 0.0f)));
    face(glm::rotate(I,   PI, glm::vec3(1.0f, 0.0f, 0.0f)));

    ComputeSize();
    MakeVAO();
}

void Box::face(const glm::mat4 tr)
{
  int n =  Pnt.size();

  float verts[8] = {1.0f,1.0f, -1.0f,1.0f, -1.0f,-1.0f, 1.0f,-1.0f};
  float texcd[8] = {1.0f,1.0f,  0.0f,1.0f,  0.0f, 0.0f, 1.0f, 0.0f};

  // Four vertices to make a single face, with its own normal and
  // texture coordinates.
  for (int i=0; i<8;  i+=2) {
      Pnt.push_back(tr*glm::vec4(verts[i], verts[i+1], 1.0f, 1.0f));
      Nrm.push_back(glm::vec3(tr*glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));
      Tex.push_back(glm::vec2(texcd[i], texcd[i+1]));
      Tan.push_back(glm::vec3(tr*glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))); }
    
  pushquad(Tri, n, n+1, n+2, n+3);
}


////////////////////////////////////////////////////////////////////////
// Generates a sphere of radius 1.0 centered at the origin.
//   n specifies the number of polygonal subdivisions
Sphere::Sphere(const int n)
{
    diffuseColor = glm::vec3(0.5, 0.5, 1.0);
    specularColor = glm::vec3(1.0, 1.0, 1.0);
    shininess = 120.0;

    float d = 2.0f*PI/float(n*2);
    for (int i=0;  i<=n*2;  i++) {
        float s = i*2.0f*PI/float(n*2);
        for (int j=0;  j<=n;  j++) {
            float t = j*PI/float(n);
            float x = cos(s)*sin(t);
            float y = sin(s)*sin(t);
            float z = cos(t);
            Pnt.push_back(glm::vec4(x,y,z,1.0f));
            Nrm.push_back(glm::vec3(x,y,z));
            Tex.push_back(glm::vec2(s/(2*PI), t/PI));
            Tan.push_back(glm::vec3(-sin(s), cos(s), 0.0));
            if (i>0 && j>0) {
                pushquad(Tri, (i-1)*(n+1) + (j-1),
                                      (i-1)*(n+1) + (j),
                                      (i  )*(n+1) + (j),
                                      (i  )*(n+1) + (j-1)); } } }
    ComputeSize();
    MakeVAO();
}

////////////////////////////////////////////////////////////////////////
// Generates a radius disk aroudn the origin in the XY plane
//   n specifies the number of polygonal subdivisions
Disk::Disk(const int n)
{
    diffuseColor = glm::vec3(0.5, 0.5, 1.0);
    specularColor = glm::vec3(1.0, 1.0, 1.0);
    shininess = 120.0;
    
    // Push center point
    Pnt.push_back(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    Nrm.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
    Tex.push_back(glm::vec2(0.5, 0.5));
    Tan.push_back(glm::vec3(1.0f, 0.0f, 0.0f));

    float d = 2.0f*PI/float(n);
    for (int i=0;  i<=n;  i++) {
        float s = i*2.0f*PI/float(n);
        float x = cos(s);
        float y = sin(s);
        Pnt.push_back(glm::vec4(x,y,0.0f,1.0f));
        Nrm.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
        Tex.push_back(glm::vec2(x*0.5+0.5, y*0.5+0.5));
        Tan.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
        if (i>0) {
          Tri.push_back(glm::ivec3(0, i+1, i)); } }
    ComputeSize();
    MakeVAO();
}

////////////////////////////////////////////////////////////////////////
// Generates a Z-aligned cylinder of radius 1 from -1 to 1 in Z
//   n specifies the number of polygonal subdivisions
Cylinder::Cylinder(const int n)
{
    diffuseColor = glm::vec3(0.5, 0.5, 1.0);
    specularColor = glm::vec3(1.0, 1.0, 1.0);
    shininess = 120.0;

    float d = 2.0f*PI/float(n);
    for (int i=0;  i<=n;  i++) {
        float s = i*2.0f*PI/float(n);
        for (int j=0;  j<=1;  j++) {
            float t = j;
            float x = cos(s);
            float y = sin(s);
            float z = t*2.0f - 1.0f;
            Pnt.push_back(glm::vec4(x,y,z,1.0f));
            Nrm.push_back(glm::vec3(x,y, 0.0f));
            Tex.push_back(glm::vec2(s/(2.0*PI), t));
            Tan.push_back(glm::vec3(-sin(s), cos(s), 0.0));
            if (i>0 && j>0) {
                pushquad(Tri, (i-1)*(2) + (j-1),
                                      (i-1)*(2) + (j),
                                      (i  )*(2) + (j),
                                      (i  )*(2) + (j-1)); } } }
    ComputeSize();
    MakeVAO();
}

////////////////////////////////////////////////////////////////////////
// Generates a plane with normals, texture coords, and tangent vectors
// from an n by n grid of small quads.  A single quad might have been
// sufficient, but that works poorly with the reflection map.
Ply::Ply(const char* name, const bool reverse)
{
    diffuseColor = glm::vec3(0.8, 0.8, 0.5);
    specularColor = glm::vec3(1.0, 1.0, 1.0);
    shininess = 120.0;

    // Open PLY file and read header;  Exit on any failure.
    p_ply ply = ply_open(name, NULL, 0, NULL);
    if (!ply) { throw std::exception(); }
    if (!ply_read_header(ply)) { throw std::exception(); }

    // Setup callback for vertices
    ply_set_read_cb(ply, "vertex", "x", vertex_cb, this, 0);
    ply_set_read_cb(ply, "vertex", "y", vertex_cb, this, 1);
    ply_set_read_cb(ply, "vertex", "z", vertex_cb, this, 2);

    ply_set_read_cb(ply, "vertex", "nx", normal_cb, this, 0);
    ply_set_read_cb(ply, "vertex", "ny", normal_cb, this, 1);
    ply_set_read_cb(ply, "vertex", "nz", normal_cb, this, 2);

    ply_set_read_cb(ply, "vertex", "s", texture_cb, this, 0);
    ply_set_read_cb(ply, "vertex", "t", texture_cb, this, 1);

    // Setup callback for faces
    ply_set_read_cb(ply, "face", "vertex_indices", face_cb, this, 0);

    // Read the PLY file filling the arrays via the callbacks.
    if (!ply_read(ply)) {printf("Failure in ply_read\n"); exit(-1); }

    ComputeSize();
    MakeVAO();
}
 

glm::vec4 staticPnt;
glm::vec3 staticNrm;
glm::vec2 staticTex;
glm::ivec3 staticTri;

// Vertex callback;  Must be static (stupid C++)
int Ply::vertex_cb(p_ply_argument argument) {
    long index;
    Ply *ply;
    ply_get_argument_user_data(argument, (void**)&ply, &index);
    double c = ply_get_argument_value(argument);
    staticPnt[index] = c;
    if (index==2) {
        staticPnt[3] = 1.0;
        ply->Pnt.push_back(staticPnt);
        ply->Tan.push_back(glm::vec3()); }
    return 1;
}
// Normal callback;  Must be static (stupid C++)
int Ply::normal_cb(p_ply_argument argument) {
    long index;
    Ply *ply;
    ply_get_argument_user_data(argument, (void**)&ply, &index);
    double c = ply_get_argument_value(argument);
    staticNrm[index] = c;
    if (index==2) {
        ply->Nrm.push_back(staticNrm); }
    return 1;
}
// Texture callback;  Must be static (stupid C++)
int Ply::texture_cb(p_ply_argument argument) {
    long index;
    Ply *ply;
    ply_get_argument_user_data(argument, (void**)&ply, &index);
    double c = ply_get_argument_value(argument);
    staticTex[index] = c;
    if (index==1) {
        ply->Tex.push_back(staticTex); }
    return 1;
}

void ComputeTangent(Ply* ply)
{
    int t = ply->Tri.size() - 1;
    int i = ply->Tri[t][0];
    int j = ply->Tri[t][1];
    int k = ply->Tri[t][2];
    glm::vec2 A = ply->Tex[i] - ply->Tex[k]; 
    glm::vec2 B = ply->Tex[j] - ply->Tex[k];
    float d = A[0]*B[1] - A[1]*B[0];
    float a =  B[1]/d;
    float b = -A[1]/d;
    glm::vec4 Tan = a*ply->Pnt[i] + b*ply->Pnt[j] + (1.0f-a-b)*ply->Pnt[k];
    ply->Tan[i] = ply->Tan[j] = ply->Tan[k] = glm::normalize(Tan.xyz());
    
}

// Face callback;  Must be static (stupid C++)
int Ply::face_cb(p_ply_argument argument) {
    long length, value_index;
    long index;
    Ply *ply;
    ply_get_argument_user_data(argument, (void**)&ply, &index);
    ply_get_argument_property(argument, NULL, &length, &value_index);

    if (value_index == -1) {
    }
    else {
        if (value_index<2) {
            staticTri[value_index] = (int)ply_get_argument_value(argument); }
        else if (value_index==2) {
            staticTri[2] = (int)ply_get_argument_value(argument);
            ply->Tri.push_back(staticTri);
            ComputeTangent(ply); }
        else if (value_index==3) {
            staticTri[1] = staticTri[2];
            staticTri[2] = (int)ply_get_argument_value(argument);
            ply->Tri.push_back(staticTri);
            ComputeTangent(ply); } }

    return 1;
}

////////////////////////////////////////////////////////////////////////
// Generates a plane with normals, texture coords, and tangent vectors
// from an n by n grid of small quads.  A single quad might have been
// sufficient, but that works poorly with the reflection map.
Plane::Plane(const float r, const int n)
{
    diffuseColor = glm::vec3(0.3, 0.2, 0.1);
    specularColor = glm::vec3(1.0, 1.0, 1.0);
    shininess = 120.0;

    for (int i=0;  i<=n;  i++) {
        float s = i/float(n);
        for (int j=0;  j<=n;  j++) {
            float t = j/float(n);
            Pnt.push_back(glm::vec4(s*2.0*r-r, t*2.0*r-r, 0.0, 1.0));
            Nrm.push_back(glm::vec3(0.0, 0.0, 1.0));
            Tex.push_back(glm::vec2(s, t));
            Tan.push_back(glm::vec3(1.0, 0.0, 0.0));
            if (i>0 && j>0) {
                pushquad(Tri, (i-1)*(n+1) + (j-1),
                                      (i-1)*(n+1) + (j),
                                      (i  )*(n+1) + (j),
                                      (i  )*(n+1) + (j-1)); } } }

    vaoID = VaoFromTris(Pnt, Nrm, Tex, Tan, Tri);
    count = Tri.size();
}

////////////////////////////////////////////////////////////////////////
// Generates a plane with normals, texture coords, and tangent vectors
// from an n by n grid of small quads.  A single quad might have been
// sufficient, but that works poorly with the reflection map.
ProceduralGround::ProceduralGround(const float _range, const int n,
                     const float _octaves, const float _persistence, const float _scale,
                     const float _low, const float _high)
    :range(_range), octaves(_octaves), persistence(_persistence), scale(_scale), 
     low(_low), high(_high)
{
    diffuseColor = glm::vec3(0.3, 0.2, 0.1);
    specularColor = glm::vec3(1.0, 1.0, 1.0);
    shininess = 10.0;
    specularColor = glm::vec3(0.0, 0.0, 0.0);
    xoff = range*( time(NULL)%1000 );

    float h = 0.001;
    for (int i=0;  i<=n;  i++) {
        float s = i/float(n);
        for (int j=0;  j<=n;  j++) {
            float t = j/float(n);
            float x = s*2.0*range-range;
            float y = t*2.0*range-range;
            float z = HeightAt(x, y);
            float zu = HeightAt(x+h, y);
            float zv = HeightAt(x, y+h);
            Pnt.push_back(glm::vec4(x, y, z, 1.0));
            glm::vec3 du(1.0, 0.0, (zu-z)/h);
            glm::vec3 dv(0.0, 1.0, (zv-z)/h);
            Nrm.push_back(glm::normalize(glm::cross(du,dv)));
            Tex.push_back(glm::vec2(s, t));
            Tan.push_back(glm::vec3(1.0, 0.0, 0.0));
            if (i>0 && j>0) {
                pushquad(Tri,
                         (i-1)*(n+1) + (j-1),
                         (i-1)*(n+1) + (j),
                         (i  )*(n+1) + (j),
                         (i  )*(n+1) + (j-1)); } } }

    vaoID = VaoFromTris(Pnt, Nrm, Tex, Tan, Tri);
    count = Tri.size();
}

float ProceduralGround::HeightAt(const float x, const float y)
{
    glm::vec3 highPoint = glm::vec3(0.0, 0.0, 0.01);

    float rs = glm::smoothstep(range-20.0f, range, sqrtf(x*x+y*y));
    float noise = scaled_octave_noise_2d(octaves, persistence, scale, low, high, x+xoff, y);
    float z = (1-rs)*noise + rs*low;
    
    float hs = glm::smoothstep(15.0f, 45.0f,
                               glm::l2Norm(glm::vec3(x,y,0)-glm::vec3(highPoint.x,highPoint.y,0)));
    return (1-hs)*highPoint.z + hs*z;
}

////////////////////////////////////////////////////////////////////////
// Generates a square divided into nxn quads;  +-1 in X and Y at Z=0
Quad::Quad(const int n)
{
    diffuseColor = glm::vec3(0.3, 0.2, 0.1);
    specularColor = glm::vec3(1.0, 1.0, 1.0);
    shininess = 120.0;

    float r = 1.0;
    for (int i=0;  i<=n;  i++) {
        float s = i/float(n);
        for (int j=0;  j<=n;  j++) {
            float t = j/float(n);
            Pnt.push_back(glm::vec4(s*2.0*r-r, t*2.0*r-r, 0.0, 1.0));
            Nrm.push_back(glm::vec3(0.0, 0.0, 1.0));
            Tex.push_back(glm::vec2(s, t));
            Tan.push_back(glm::vec3(1.0, 0.0, 0.0));
            if (i>0 && j>0) {
                pushquad(Tri,
                         (i-1)*(n+1) + (j-1),
                         (i-1)*(n+1) + (j),
                         (i  )*(n+1) + (j),
                         (i  )*(n+1) + (j-1)); } } }

    vaoID = VaoFromTris(Pnt, Nrm, Tex, Tan, Tri);
    count = Tri.size();
}
