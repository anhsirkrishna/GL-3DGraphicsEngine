/////////////////////////////////////////////////////////////////////////
// Pixel shader for Final output
////////////////////////////////////////////////////////////////////////
#version 330

const float PI = 3.14159f;

out vec4 FragColor;

uniform sampler2D gBufferWorldPos;
uniform sampler2D gBufferNormalVec;

uniform int width, height;
uniform int ao_sample_count;
uniform float range_of_influence;

uniform float scale;
uniform float contrast;

void main()
{
    //Following lines of code all read in values from the gbuffer
    //=================================================================
    vec2 uv = gl_FragCoord.xy / vec2(width, height);
    vec4 worldPos = texture2D(gBufferWorldPos, uv);
    float pixel_depth = worldPos.w;
    vec3 normalVec = texture2D(gBufferNormalVec, uv).xyz;
    //=================================================================


    //Ambient occlusion factor calculation
    float S = 0;
    float R = range_of_influence;
    vec3 omega_i;
    vec3 P = worldPos.xyz;
    vec3 Pi;
    float Di;
    float alpha;
    float h;
    float theta;
    float phi = (30 * int(gl_FragCoord.x) ^ int(gl_FragCoord.y)) + (10*gl_FragCoord.x*gl_FragCoord.y);
    vec2 uv_i;
    float c = 0.1*R;
    float delta = 0.001;
    vec4 Pos_i;

    for (int i=0; i < ao_sample_count; ++i) {
        //Choosing a neighboring point
        //=================================================================
        alpha = (i+0.5)/ao_sample_count;
        h = (alpha*R)/pixel_depth;
        theta = (2*PI*alpha*((7*ao_sample_count)/9)) + phi;
        uv_i = uv + h*vec2(cos(theta), sin(theta));
        Pos_i = texture2D(gBufferWorldPos, uv_i);
        Pi = Pos_i.xyz;
        Di = Pos_i.w;
        //=================================================================

        //Calculating factor S
        //=================================================================
        omega_i = P - Pi;
        if ((R - length(omega_i)) < 0) {
            S += 0;
        }
        else {
            S += max(0, dot(normalVec, omega_i) - delta*Di) / max(c*c, dot(omega_i, omega_i));
        }
            
        //=================================================================
    }
    S = S * ((2*PI*c)/ao_sample_count);
    
    FragColor = vec4(max(0, pow((1 - (scale*S)), contrast)));
}
