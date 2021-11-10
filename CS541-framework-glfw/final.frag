/////////////////////////////////////////////////////////////////////////
// Pixel shader for Final output
////////////////////////////////////////////////////////////////////////
#version 330

const float PI = 3.14159f;

out vec4 FragColor;

uniform bool reflective;
uniform sampler2D upperReflectionMap, lowerReflectionMap;
uniform int reflectionMode;
uniform vec3 diffuse, specular;
uniform float shininess;

in vec3 normalVec, lightVec, eyeVec;

vec3 LightingPixel();
void main()
{
    vec3 outColor = LightingPixel();
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
