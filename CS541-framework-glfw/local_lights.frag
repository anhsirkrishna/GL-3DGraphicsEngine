/////////////////////////////////////////////////////////////////////////
// Pixel shader for Local lights
////////////////////////////////////////////////////////////////////////
#version 330

const float PI = 3.14159f;
const float exposure = 2;

const int Phong_M = 0;
const int BRDF_M = 1;
const int GGX_M = 2;
const int IBL_M = 3;

uniform mat4 WorldInverse;

uniform vec3 localLightPos;
uniform vec3 diffuse;
uniform vec3 ambient;

uniform sampler2D gBufferWorldPos;
uniform sampler2D gBufferNormalVec;
uniform sampler2D gBufferDiffuse;
uniform sampler2D gBufferSpecular;

uniform float localLightRadius;

uniform int width, height;
uniform int lightingMode;


void main()
{
    //Following lines of code all read in values from the gbuffer
    //=================================================================
    vec2 uv = gl_FragCoord.xy / vec2(width, height);
    vec4 worldPos = texture2D(gBufferWorldPos, uv);
    vec3 normalVec = texture2D(gBufferNormalVec, uv).xyz;
    vec3 Kd = texture2D(gBufferDiffuse, uv).xyz;
    vec3 Ks = texture2D(gBufferSpecular, uv).xyz;
    float shininess = texture2D(gBufferSpecular, uv).w;
    //=================================================================
    vec3 eyePos = (WorldInverse*vec4(0, 0, 0, 1)).xyz;

    vec3 outColor = vec3(0);
    float alpha;
    vec3 light = diffuse;
    //Check if the world position corresponding to this pixel 
    // is in range of this particular local light
    vec3 local_light_vec = localLightPos - worldPos.xyz;
    float light_distance_squared = dot(local_light_vec, local_light_vec);
    float local_radius_squared = pow(localLightRadius, 2);
    if (light_distance_squared < local_radius_squared) {
        //Inside of local light influence. Do additive local light caclulation

        vec3 N = normalize(normalVec);
        vec3 L = normalize(local_light_vec);
        vec3 V = normalize(eyePos - worldPos.xyz);
        vec3 H = normalize(L+V);

        float LN = max(dot(L, N), 0.0);
        float NH = max(dot(N, H), 0.0);
        float LH = max(dot(L, H), 0.0);

        if (lightingMode == Phong_M){
            alpha = -2 + (2/(shininess*shininess));
            outColor = ambient*Kd + light*(Kd/PI)*LN + light*(Ks*10)*pow(NH, alpha);
            //Ks value coming in is for BRDF so adjust for Phong by multiplying by 10
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
            outColor = light*LN*brdf * max((1/light_distance_squared) - (1/local_radius_squared), 0);
        }
    }
    gl_FragColor.xyz = outColor;
}