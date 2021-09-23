/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 330

out vec4 FragColor;

// These definitions agree with the ObjectIds enum in scene.h
const int     nullId	= 0;
const int     skyId	= 1;
const int     seaId	= 2;
const int     groundId	= 3;
const int     roomId	= 4;
const int     boxId	= 5;
const int     frameId	= 6;
const int     lPicId	= 7;
const int     rPicId	= 8;
const int     teapotId	= 9;
const int     spheresId	= 10;
const int     floorId	= 11;

in vec3 normalVec, lightVec;
in vec2 texCoord;

uniform int objectId;
uniform vec3 diffuse;

void main()
{
    vec3 N = normalize(normalVec);
    vec3 L = normalize(lightVec);

    vec3 Kd = diffuse;   

    // A checkerboard pattern to break up larte flat expanses.  Remove when using textures.
    if (objectId==groundId || objectId==floorId || objectId==seaId) {
        ivec2 uv = ivec2(floor(100.0*texCoord));
        if ((uv[0]+uv[1])%2==0)
            Kd *= 0.9; }
    
    FragColor.xyz = vec3(0.5,0.5,0.5)*Kd + Kd*max(dot(L,N),0.0);
}
