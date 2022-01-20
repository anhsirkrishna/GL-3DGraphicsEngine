/////////////////////////////////////////////////////////////////////////
// Pixel shader for Final output
////////////////////////////////////////////////////////////////////////
#version 330

const float PI = 3.14159f;

out vec4 FragColor;

uniform bool reflective;
uniform sampler2D upperReflectionMap;
uniform sampler2D lowerReflectionMap;
uniform sampler2D shadowMap;
uniform int reflectionMode;
uniform int drawFbo;
uniform vec3 diffuse, specular;
uniform float shininess;
uniform int width, height;

in vec3 normalVec, lightVec, eyeVec;

in vec4 shadowCoord;

vec3 LightingPixel();
void main()
{
    vec3 outColor = LightingPixel();
    vec2 uv;

    uv.x = gl_FragCoord.x/width;
    uv.y = gl_FragCoord.y/height;

    if (drawFbo == 0){
        vec2 shadowIndex;
        if (shadowCoord.w > 0){
            shadowIndex = shadowCoord.xy/shadowCoord.w;
        }
        float pixel_depth = 0;
        float light_depth = 1000;
        if (shadowIndex.x >= 0 && shadowIndex.x <= 1){
            if (shadowIndex.y >= 0 && shadowIndex.y <= 1){
                light_depth = texture2D(shadowMap, shadowIndex).w;
                pixel_depth = shadowCoord.w;
            }
        }
        light_depth = texture2D(shadowMap, uv).w;
        light_depth = light_depth/800;
        FragColor.x = light_depth;
        FragColor.y = FragColor.x;
        FragColor.z = FragColor.x;
        return;
    }
    if (drawFbo == 1){
        FragColor.xyz = texture2D(upperReflectionMap, uv).xyz;
        return;
    }
    if (drawFbo == 2){
        FragColor.xyz = texture2D(lowerReflectionMap, uv).xyz;
        return;
    }
    
    if (reflective){
        vec3 reflectionColor;
        vec3 N = normalize(normalVec);
        vec3 V = normalize(eyeVec);
        vec3 R = (2*dot(N, V)*N) - V;
        vec3 H = normalize(R+V);
        float RH = max(dot(R, H), 0.0);
        float NH = max(dot(N, H), 0.0);
        float NR = max(dot(N, R), 0.0);
        vec3 abc = normalize(R);
        vec2 uv;
        if (abc.z > 0)
        {
            uv = vec2(abc.x/(1+abc.z), abc.y/(1+abc.z))*0.5 + vec2(0.5, 0.5);
            reflectionColor = texture2D(upperReflectionMap, uv).xyz;
        }
        else
        {
            uv = vec2(abc.x/(1-abc.z), abc.y/(1-abc.z))*0.5 + vec2(0.5, 0.5);
            reflectionColor = texture2D(lowerReflectionMap, uv).xyz;
        }
        
        vec3 brdf;
        float distribution;
        
        vec3 fresnel = specular + ((vec3(1,1,1) - specular)*pow((1-RH), 5));
        float visibility = 1/(RH*RH);
        float alpha = -2 + (2/(shininess*shininess));
        distribution = ((alpha+2)/(2*PI))*pow(NH, alpha);

        brdf = (diffuse/PI) + ((fresnel*visibility*distribution)/4);

        if (reflectionMode == 0) //Mirror-like
            FragColor.xyz = reflectionColor;
        else if (reflectionMode == 1) //Color mixed with brdf
            FragColor.xyz = outColor + reflectionColor*NR*brdf;
        else if (reflectionMode == 2) //Color mixed simple
            FragColor.xyz = outColor + reflectionColor*0.1;
        else
            FragColor.xyz = outColor;
    }
    else
        FragColor.xyz = outColor;
}
