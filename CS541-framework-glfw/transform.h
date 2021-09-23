////////////////////////////////////////////////////////////////////////
// A small library of 4x4 matrix operations needed for graphics
// transformations.  glm::mat4 is a 4x4 float matrix class with indexing
// and printing methods.  A small list or procedures are supplied to
// create Rotate, Scale, Translate, and Perspective matrices and to
// return the product of any two such.

#ifndef _TRANSFORM_
#define _TRANSFORM_

#include <fstream>

// Factory functions to create specific transformations, multiply two, and invert one.
glm::mat4 Rotate(const int i, const float theta);
glm::mat4 Scale(const float x, const float y, const float z);
glm::mat4 Translate(const float x, const float y, const float z);
glm::mat4 Perspective(const float rx, const float ry,
                 const float front, const float back);
glm::mat4 LookAt(const glm::vec3 Eye, const glm::vec3 Center, const glm::vec3 Up);

float* Pntr(glm::mat4& m);

#endif
