/////////////////////////////////////////////////////////////////////////
// Vertex shader for Final output
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform mat4 WorldView, WorldProj, ModelTr, NormalTr;

in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexture;
in vec3 vertexTangent;

out vec4 worldPos;
out vec3 normalVec;
void main()
{
    worldPos = ModelTr*vertex;
    gl_Position = WorldProj*WorldView*worldPos;
    normalVec = vertexNormal*mat3(NormalTr);
}
