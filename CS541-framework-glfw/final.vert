/////////////////////////////////////////////////////////////////////////
// Vertex shader for Final output
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform mat4 WorldView, WorldInverse, WorldProj, ModelTr;

in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexture;
in vec3 vertexTangent;

void LightingVertex(vec3 Eye);
void main()
{      
    gl_Position = WorldProj*WorldView*ModelTr*vertex;
	vec3 eyePos = (WorldInverse*vec4(0, 0, 0, 1)).xyz;

	LightingVertex(eyePos);
}
