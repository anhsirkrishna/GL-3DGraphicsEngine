/////////////////////////////////////////////////////////////////////////
// Vertex shader for lighting
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform mat4 ModelTr, NormalTr, ShadowMatrix;
uniform vec3 lightPos;

in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexture;
in vec3 vertexTangent;

out vec3 normalVec, lightVec, eyeVec;
out vec2 texCoord;
out vec3 tanVec;
out vec4 shadowCoord;

void LightingVertex(vec3 Eye)
{
    shadowCoord = ShadowMatrix*ModelTr*vertex;
    vec3 worldPos = (ModelTr*vertex).xyz;

    tanVec = mat3(ModelTr)*vertexTangent;

    normalVec = vertexNormal*mat3(NormalTr); 
    lightVec = lightPos - worldPos;
    eyeVec = Eye - worldPos;

    texCoord = vertexTexture; 
}
