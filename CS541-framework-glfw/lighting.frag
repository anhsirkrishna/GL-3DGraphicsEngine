/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 330

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
const int IBL_M = 3;

const float PI = 3.14159f;
const float exposure = 3;

in vec3 normalVec, lightVec, eyeVec;
in vec2 texCoord;
in vec4 shadowCoord;
in vec3 tanVec;

uniform int objectId;
uniform int lightingMode;
uniform vec3 diffuse, specular;
uniform float shininess;
uniform vec3 light, ambient;
uniform int width, height;
uniform bool reflective;

uniform sampler2D shadowMap;
uniform sampler2D SkydomeTex;
uniform sampler2D IrrMapTex;
uniform sampler2D ObjectTexture;
uniform sampler2D ObjectNMap;
uniform int hasTexture, hasNMap;

vec3 LightingPixel()
{
    vec3 N = normalize(normalVec);
    vec3 L = normalize(lightVec);
    vec3 V = normalize(eyeVec);
    vec3 H = normalize(L+V);

    vec3 returnColor;
    vec3 Kd = diffuse;

    if (objectId == skyId){
        vec2 uv = vec2(-atan(V.y, V.x)/(2*PI), acos(V.z)/PI);
        returnColor = texture2D(SkydomeTex, uv).xyz;
        return returnColor;
    }
    else if (objectId == seaId){
        vec3 R = -(2* dot(V, N) * N - V);
        vec2 uv = vec2(-atan(R.y, R.x)/(2*PI), acos(R.z)/PI);
        Kd = texture2D(SkydomeTex, uv).xyz;
    }

    if (hasTexture != -1){
        //Get color from texture
        vec2 uv = texCoord;

        if (objectId == groundId)
            uv *= 10;
        if (objectId == floorId)
            uv *= 5;
        if (objectId == roomId){
            uv = uv.yx;
            uv *= 50;
        }
        if (objectId == rPicId){
            if (texCoord.x < 0.1){
                Kd = vec3(0.7, 0.7, 0.7);
            }
            else if (texCoord.y < 0.1){
                Kd = vec3(0.7, 0.7, 0.7);
            }
            else if (texCoord.x > 0.9){
                Kd = vec3(0.7, 0.7, 0.7);
            }
            else if (texCoord.y > 0.9){
                Kd = vec3(0.7, 0.7, 0.7);
            }  
            else{
                uv.x -= 0.05;
                uv.y -= 0.1;
                uv /= 0.8;
                Kd = texture2D(ObjectTexture, uv).xyz;
            }
        }
        else if (objectId == lPicId){
            uv *= 100;
            if ((mod(uv.x, 20) > 5) && mod(uv.y, 20) > 5)
                Kd = vec3(0.6, 0.0, 0.9);
            else if ( uv.x < (uv.y + 10) && uv.x > (uv.y - 10))
                Kd = vec3(0,0,0);
            else
                Kd = vec3(0.3, 0.0, 0.0);
        }
        else
            Kd = texture2D(ObjectTexture, uv).xyz;
    }

    if (hasNMap != -1){
        vec2 uv = texCoord;
        if (objectId == groundId)
            uv *= 10;
        if (objectId == floorId)
            uv *= 5;
        if (objectId == roomId){
            uv = uv.yx;
            uv *= 50;
        }
        if (objectId == seaId){
            uv = uv.yx;
            uv *= 500;
        }
        vec3 delta = texture2D(ObjectNMap, uv).xyz;
        delta = delta*2.0 - vec3(1, 1, 1);
        vec3 T = normalize(tanVec);
        vec3 B = normalize(cross(T, N));
        N = delta.x*T + delta.y*B + delta.z*N;
        N = normalize(N);
    }
        
    float alpha;
    float light_depth, pixel_depth;
    vec2 shadowIndex;
    if (shadowCoord.w > 0){
        shadowIndex = shadowCoord.xy/shadowCoord.w;
    }
    pixel_depth = 0;
    light_depth = 1000;
    if (shadowIndex.x >= 0 && shadowIndex.x <= 1){
        if (shadowIndex.y >= 0 && shadowIndex.y <= 1){
            light_depth = texture2D(shadowMap, shadowIndex).w;
            pixel_depth = shadowCoord.w;
        }
    }
    // A checkerboard pattern to break up larte flat expanses.  Remove when using textures.
    //if (objectId==groundId || objectId==floorId || objectId==seaId) {
    //    ivec2 uv = ivec2(floor(100.0*texCoord));
    //    if ((uv[0]+uv[1])%2==0)
    //        Kd *= 0.9; }
    
    //Convert colors into linear color space for calculations
    Kd = (exposure*Kd)/(exposure*Kd + 1);
    Kd.r = pow(Kd.r, 2.2);
    Kd.g = pow(Kd.g, 2.2);
    Kd.b = pow(Kd.b, 2.2);

    vec3 Ks = (exposure*specular)/(exposure*specular +1);
    Ks.r = pow(Ks.r, 2.2);
    Ks.g = pow(Ks.g, 2.2);
    Ks.b = pow(Ks.b, 2.2);


    float LN = max(dot(L, N), 0.0);
    float NH = max(dot(N, H), 0.0);
    float LH = max(dot(L, H), 0.0);
    if (lightingMode == Phong_M){
        alpha = -2 + (2/(shininess*shininess));
        if (pixel_depth > (light_depth + 0.01))
            returnColor = ambient*Kd;
        else
            returnColor = ambient*Kd + light*(Kd/PI)*LN + light*(Ks*10)*pow(NH, alpha);
        //Ks value coming in is for BRDF so adjust for Phong by multiplying by 10
    }
    else if (lightingMode == IBL_M){
        vec2 uv = vec2(-atan(-N.y, -N.x)/(2*PI), acos(-N.z)/PI);
        vec3 irr_map_color = texture2D(IrrMapTex, uv).xyz;
        irr_map_color = (exposure*irr_map_color)/(exposure*irr_map_color + 1);
        irr_map_color.r = pow(irr_map_color.r, 2.2);
        irr_map_color.g = pow(irr_map_color.g, 2.2);
        irr_map_color.b = pow(irr_map_color.b, 2.2);

        vec3 R = normalize((2*dot(N, V)*N) - V);
        vec3 newH = normalize(R+V);
        uv = vec2(-atan(-R.y, -R.x)/(2*PI), acos(-R.z)/PI);
        vec3 reflection_map_color = texture2D(SkydomeTex, uv).xyz;
        
        vec3 brdf;
        float distribution;
        float RH = max(dot(R, newH), 0);
        float NR = max(dot(N, R), 0.0);
        float newNH = max(dot(N, newH), 0);
        vec3 fresnel = Ks + ((vec3(1,1,1) - Ks)*pow((1-RH), 5));
        float visibility = 1/(RH*RH);
        float alpha = -2 + (2/(shininess*shininess));
        distribution = ((alpha+2)/(2*PI))*pow(newNH, alpha);

        brdf = ((fresnel*visibility*distribution)/4);

        reflection_map_color = ((exposure* reflection_map_color)/(exposure*reflection_map_color + 1));
        reflection_map_color.r = pow(reflection_map_color.r, 2.2);
        reflection_map_color.g = pow(reflection_map_color.g, 2.2);
        reflection_map_color.b = pow(reflection_map_color.b, 2.2);
        returnColor = (Kd/PI)*irr_map_color + (reflection_map_color * brdf * NR);
        //returnColor = (Kd/PI)*irr_map_color;
    }
    else {
        vec3 brdf;
        float distribution;
        
        vec3 fresnel = Ks + ((vec3(1,1,1) - Ks)*pow((1-LH), 5));
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
        if (pixel_depth > (light_depth + 0.01))
            returnColor = ambient*Kd;
        else
            returnColor = ambient*Kd + light*LN*brdf;
    }

    //Convert color back into sRGB space
    returnColor = (exposure*returnColor)/(exposure*returnColor + 1);
    returnColor.r = pow(returnColor.r, 1/2.2);
    returnColor.g = pow(returnColor.g, 1/2.2);
    returnColor.b = pow(returnColor.b, 1/2.2);

	return returnColor;
}
