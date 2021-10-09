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

const int Phong_M = 0;
const int BRDF_M = 1;
const int GGX_M = 2;

const float PI = 3.14159f;

in vec3 normalVec, lightVec, eyeVec;
in vec2 texCoord;

uniform int objectId;
uniform int lightingMode;
uniform vec3 diffuse, specular;
uniform float shininess;
uniform vec3 light, ambient;

void main()
{
    vec3 N = normalize(normalVec);
    vec3 L = normalize(lightVec);
    vec3 V = normalize(eyeVec);
    vec3 H = normalize(L+V);

    vec3 Kd = diffuse;   
    float alpha;
    // A checkerboard pattern to break up larte flat expanses.  Remove when using textures.
    if (objectId==groundId || objectId==floorId || objectId==seaId) {
        ivec2 uv = ivec2(floor(100.0*texCoord));
        if ((uv[0]+uv[1])%2==0)
            Kd *= 0.9; }
    
    float LN = max(dot(L, N), 0.0);
    float NH = max(dot(N, H), 0.0);
    float LH = max(dot(L, H), 0.0);
    if (lightingMode == Phong_M){
        alpha = -2 + (2/(shininess*shininess));
        FragColor.xyz = ambient*Kd + light*(Kd/PI)*LN + light*(specular*10)*pow(NH, alpha); 
        //specular value coming in is for BRDF so adjust for Phong by multiplying by 10
    }
    else {
        vec3 brdf;
        float distribution;
        
        vec3 fresnel = specular + ((vec3(1,1,1) - specular)*pow((1-LH), 5));
        float visibility = 1/(LH*LH);

        if (lightingMode == BRDF_M){
            alpha = -2 + (2/(shininess*shininess));
            distribution = ((alpha+2)/(2*PI))*pow(NH, alpha);
        }
        else if (lightingMode == GGX_M){
            alpha = pow(shininess, 2);
            distribution = alpha / (PI * pow(pow(NH, 2) * (alpha - 1) + 1, 2));
        }

        brdf = (Kd/PI) + ((fresnel*visibility*distribution)/4);
        FragColor.xyz = ambient*Kd + light*LN*brdf; 
    }
}
