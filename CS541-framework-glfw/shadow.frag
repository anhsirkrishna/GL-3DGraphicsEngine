/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 330

out vec4 FragColor;

const float PI = 3.14159f;

in vec3 normalVec, lightVec, eyeVec;
in vec2 texCoord;
in vec4 position;

uniform int objectId;
uniform int lightingMode;
uniform vec3 diffuse, specular;
uniform float shininess;
uniform vec3 light, ambient;

void main()
{
    gl_FragData[0] = position;
}
