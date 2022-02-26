/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 330

const float PI = 3.14159f;

in vec4 position;

uniform int debugMode;

void main()
{
    gl_FragData[0].x = position.w;
    gl_FragData[0].y = pow(position.w, 2);
    gl_FragData[0].z = pow(position.w, 3);
    gl_FragData[0].w = pow(position.w, 4);
}
