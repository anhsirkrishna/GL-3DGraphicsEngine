/////////////////////////////////////////////////////////////////////////
// Pixel shader for Final output
////////////////////////////////////////////////////////////////////////
#version 330

const float PI = 3.14159f;

uniform vec3 diffuse, specular;
uniform float shininess;

in vec4 worldPos;
in vec3 normalVec;

void main()
{
	gl_FragData[0] = worldPos;
	gl_FragData[1].xyz = normalVec;
	gl_FragData[2].xyz = diffuse;
	gl_FragData[3].xyz = specular;
	gl_FragData[3].w = shininess;
}
