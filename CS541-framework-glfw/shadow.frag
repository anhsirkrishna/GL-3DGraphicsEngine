/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 330

const float PI = 3.14159f;

in vec4 position;

uniform int debugMode;
uniform float min_depth, max_depth;

void main()
{
    float relative_depth = (position.w - min_depth)/ (max_depth - min_depth);
    gl_FragData[0].x = relative_depth;
    gl_FragData[0].y = pow(relative_depth, 2);
    gl_FragData[0].z = pow(relative_depth, 3);
    gl_FragData[0].w = pow(relative_depth, 4);
}
