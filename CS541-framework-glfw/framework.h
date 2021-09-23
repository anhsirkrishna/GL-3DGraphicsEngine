// All the includes neeed across several cpp files:
//   OpenGL, GLFW, GLM, IMGUI, etc

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#define GLFW_INCLUDE_NONE
#include <glbinding/Binding.h>  // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>

#include "shader.h"
#include "scene.h"
#include "interact.h"
