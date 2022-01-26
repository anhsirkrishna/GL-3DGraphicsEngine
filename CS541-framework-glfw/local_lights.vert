/////////////////////////////////////////////////////////////////////////
// Vertex shader for local lights
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform mat4 ModelTr, WorldView, WorldProj;

in vec4 vertex;
out vec3 lightPos;
void main()
{
    lightPos = (ModelTr * vec4(0,0,0,1)).xyz;

    gl_Position = WorldProj * WorldView * ModelTr * vertex;
}
