/////////////////////////////////////////////////////////////////////////
// Vertex shader for reflection shader
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform mat4 ModelTr;
uniform vec3 ReflectionEye;
uniform int HemisphereSign;

in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexture;
in vec3 vertexTangent;

void LightingVertex(vec3 Eye);
void main()
{   
	vec4 P = ModelTr*vertex;
	vec3 R = P.xyz - ReflectionEye;
	vec3 abc = normalize(R);
    gl_Position = vec4(abc.x/(1+(abc.z*HemisphereSign)), 
					   abc.y/(1+(abc.z*HemisphereSign)),
					   (abc.z*HemisphereSign*length(R)/1000) - 1.0,
					   1.0);

	LightingVertex(ReflectionEye);
}
