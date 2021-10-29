////////////////////////////////////////////////////////////////////////
// A small library of 4x4 matrix operations needed for graphics
// transformations.  glm::mat4 is a 4x4 float matrix class with indexing
// and printing methods.  A small list or procedures are supplied to
// create Rotate, Scale, Translate, and Perspective matrices and to
// return the product of any two such.

#include <glm/glm.hpp>

#include "math.h"
#include "transform.h"

float* Pntr(glm::mat4& M)
{
    return &(M[0][0]);
}

//@@ The following procedures should calculate and return 4x4
//transformation matrices instead of the identity.

// Return a rotation matrix around an axis (0:X, 1:Y, 2:Z) by an angle
// measured in degrees.  NOTE: Make sure to convert degrees to radians
// before using sin and cos.  HINT: radians = degrees*PI/180
const float pi = 3.14159f;
glm::mat4 Rotate(const int i, const float theta)
{
    glm::mat4 R;
    switch (i) {
        case 0:
            R[0].x = 1;
            R[0].y = 0;
            R[0].z = 0;
            R[1].x = 0;
            R[1].y = cos((theta * pi) / 180);
            R[1].z = sin((theta * pi) / 180);
            R[2].x = 0;
            R[2].y = -1 * sin((theta * pi) / 180);
            R[2].z = cos((theta * pi) / 180);
            break;
        case 1:
            R[0].x = cos((theta * pi) / 180);
            R[0].y = 0;
            R[0].z = -1 * sin((theta * pi) / 180);
            R[1].x = 0;
            R[1].y = 1;
            R[1].z = 0;
            R[2].x = sin((theta * pi) / 180);
            R[2].y = 0;
            R[2].z = cos((theta * pi) / 180);
            break;
        case 2:
            R[0].x = cos((theta * pi) / 180);
            R[0].y = sin((theta * pi) / 180);
            R[0].z = 0;
            R[1].x = -1 * sin((theta * pi) / 180);
            R[1].y = cos((theta * pi) / 180);
            R[1].z = 0;
            R[2].x = 0;
            R[2].y = 0;
            R[2].z = 1;
            break;
    }

    return R;
}

// Return a scale matrix
glm::mat4 Scale(const float x, const float y, const float z)
{
    glm::mat4 S;
    S[0].x = x;
    S[1].y = y;
    S[2].z = z;
    return S;
}

// Return a translation matrix
glm::mat4 Translate(const float x, const float y, const float z)
{
    glm::mat4 T;
    T[3] = glm::vec4(glm::vec3(x, y, z), 1.0f);
    return T;
}

// Returns a perspective projection matrix
glm::mat4 Perspective(const float rx, const float ry,
             const float front, const float back)
{
    glm::mat4 P;
    P[0].x = 1 / rx;
    P[1].y = 1 / ry;
    P[2].z = (-1 * (back + front)) / (back - front);
    P[2].w = -1;
    P[3].z = (-2*(back * front)) / (back - front);
    P[3].w = 0;
    return P;
}

glm::mat4 LookAt(const glm::vec3 Eye, const glm::vec3 Center, const glm::vec3 Up) {
    glm::vec3 V, A, B;
    glm::mat4 rotMat;
    V = glm::normalize(Center - Eye);
    A = glm::normalize(glm::cross(V, Up));
    B = glm::cross(A, V);

    rotMat[0].x = A.x;
    rotMat[1].x = A.y;
    rotMat[2].x = A.z;

    rotMat[0].y = B.x;
    rotMat[1].y = B.y;
    rotMat[2].y = B.z;

    rotMat[0].z = -V.x;
    rotMat[1].z = -V.y;
    rotMat[2].z = -V.z;

    rotMat[3].w = 1;

    return rotMat * Translate(-Eye.x, -Eye.y, -Eye.z);
}