/////////////////////////////////////////////////////////////////////////
// Vertex shader for lighting
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform mat4 LightView, WorldProj, ModelTr;

in vec4 vertex;
out vec4 position;

void main()
{      
    gl_Position = WorldProj*LightView*ModelTr*vertex;
    
    position = gl_Position;

}
