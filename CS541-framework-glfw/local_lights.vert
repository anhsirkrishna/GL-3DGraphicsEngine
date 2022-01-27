/////////////////////////////////////////////////////////////////////////
// Vertex shader for local lights
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform mat4 ModelTr, WorldView, WorldProj;

in vec4 vertex;

void main()
{
    gl_Position = WorldProj * WorldView * ModelTr * vertex;
}
