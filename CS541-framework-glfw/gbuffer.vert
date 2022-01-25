/////////////////////////////////////////////////////////////////////////
// Vertex shader for Final output
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform mat4 WorldView, WorldProj, ModelTr, NormalTr, WorldInverse;
uniform vec3 lightPos;

in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexture;
in vec3 vertexTangent;

out vec4 worldPos;
out vec3 normalVec;
out vec2 texCoord;
out vec3 tanVec;
out vec4 shadowCoord;
out vec3 eyeVec;

void main()
{
    worldPos = ModelTr*vertex;
    normalVec = vertexNormal*mat3(NormalTr);

    tanVec = mat3(ModelTr)*vertexTangent;
    vec3 lightVec = lightPos - worldPos.xyz;
    vec3 eyePos = (WorldInverse*vec4(0, 0, 0, 1)).xyz;
    eyeVec = eyePos - worldPos.xyz;

    texCoord = vertexTexture;

    gl_Position = WorldProj*WorldView*worldPos;
}
