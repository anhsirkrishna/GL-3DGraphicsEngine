/////////////////////////////////////////////////////////////////////////
// Pixel shader for Final output
////////////////////////////////////////////////////////////////////////
#version 330

const float PI = 3.14159f;
const float exposure = 2;

const int Phong_M = 0;
const int BRDF_M = 1;
const int GGX_M = 2;
const int IBL_M = 3;

out vec4 FragColor;

uniform mat4 ShadowMatrix, WorldInverse;

uniform vec3 lightPos;
uniform vec3 light, ambient;

uniform sampler2D upperReflectionMap;
uniform sampler2D lowerReflectionMap;
uniform sampler2D gBufferWorldPos;
uniform sampler2D gBufferNormalVec;
uniform sampler2D gBufferDiffuse;
uniform sampler2D gBufferSpecular;
uniform sampler2D shadowMap;
uniform sampler2D SkydomeTex;
uniform sampler2D IrrMapTex;
uniform int reflectionMode;
uniform int textureMode;
uniform int lightingMode;
uniform int drawFbo;
uniform float shininess;
uniform int width, height;
uniform float min_depth, max_depth;

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
    bool reflective = true;
    if (texture2D(gBufferDiffuse, uv).w == 0)
        reflective = false;

    bool isSkyDome = false;
    if (texture2D(gBufferNormalVec, uv).w == 1){
        isSkyDome = true;
        FragColor.xyz = Kd;
        return;
    }
        

    vec4 shadowCoord = ShadowMatrix*worldPos;
    vec3 eyePos = (WorldInverse*vec4(0, 0, 0, 1)).xyz;
    //=================================================================


    //Following lines of code all contribute towards the debug drawings
    //=================================================================

    if (drawFbo == 0){
        float light_depth;
        light_depth = texture2D(shadowMap, uv).x;
        light_depth = light_depth;
        FragColor.x = light_depth;
        FragColor.y = FragColor.x;
        FragColor.z = FragColor.x;
        return;
    }
    if (drawFbo == 1){
        float light_depth;
        light_depth = texture2D(shadowMap, uv).y;
        light_depth = light_depth;
        FragColor.x = light_depth;
        FragColor.y = FragColor.x;
        FragColor.z = FragColor.x;
        return;
    }
    if (drawFbo == 2){
        float light_depth;
        light_depth = texture2D(shadowMap, uv).z;
        light_depth = light_depth;
        FragColor.x = light_depth;
        FragColor.y = FragColor.x;
        FragColor.z = FragColor.x;
        return;
    }
    if (drawFbo == 3){
        float light_depth;
        light_depth = texture2D(shadowMap, uv).w;
        light_depth = light_depth;
        FragColor.x = light_depth;
        FragColor.y = FragColor.x;
        FragColor.z = FragColor.x;
        return;
    }
    if (drawFbo == 4){
        FragColor.xyz = texture2D(upperReflectionMap, uv).xyz;
        return;
    }
    if (drawFbo == 5){
        FragColor.xyz = texture2D(lowerReflectionMap, uv).xyz;
        return;
    }
    if (drawFbo == 6){
        FragColor.xyz = texture2D(gBufferWorldPos, uv).xyz/100;
        return;
    }
    if (drawFbo == 7){
        FragColor.xyz = abs(texture2D(gBufferNormalVec, uv).xyz);
        return;
    }
    if (drawFbo == 8){
        FragColor.xyz = texture2D(gBufferDiffuse, uv).xyz;
        return;
    }
    if (drawFbo == 9){
        FragColor.xyz = texture2D(gBufferSpecular, uv).xyz;
        return;
    }
    
    //=================================================================
    //End of debug drawing code. What follows now is actual lighting calculations

    vec3 outColor;

    float alpha;
    float light_depth, pixel_depth;
    vec2 shadowIndex;
    vec4 shadow_b;
    if (shadowCoord.w > 0){
        shadowIndex = shadowCoord.xy/shadowCoord.w;
    }
    pixel_depth = 0;
    light_depth = 1000;
    if (shadowIndex.x >= 0 && shadowIndex.x <= 1){
        if (shadowIndex.y >= 0 && shadowIndex.y <= 1){
            shadow_b = texture2D(shadowMap, shadowIndex);
            light_depth = shadow_b.x;
            pixel_depth = shadowCoord.w;
        }
    }
    
    //Convert absolute pixel_depth into relative pixel_depth
    pixel_depth = (pixel_depth - min_depth) / (max_depth - min_depth);

    //Blurred shadow algorithm
    alpha =  3.0f/100000.0f;
    vec4 b_prime = ((1 - alpha) * shadow_b) + (alpha * vec4(0.5f));


    //Chebychev alg
    float m1 = shadow_b[0];
    float m2 = shadow_b[1];

    float mean = m1;
    float variance = (m2 - pow(m1, 2));

    float S = variance/(variance + pow((pixel_depth - mean), 2));

    //Cholesky decomposition
    float a = 1;
    float b = b_prime[0]/a;
    float c = b_prime[1]/a;
    float d = sqrt(b_prime[1] - (b * b));
    float e = (b_prime[2] - (b*c))/d;
    float f = sqrt(b_prime[3] - (c*c) - (e*e));

    float c1_hat = 1/a;
    float c2_hat = (pixel_depth - (b*c1_hat))/d;
    float c3_hat = ((pixel_depth *  pixel_depth) - (c*c1_hat) - (e*c2_hat))/f;

    float c3 = c3_hat/f;
    float c2 = (c2_hat - (e*c3))/d;
    float c1 = (c1_hat - (b*c2) - (c*c3))/a;


    //Quadracting solving for z2 and z3
    // c3(z^2) + c2(z) + c1 = 0
    float root_1 = (-c2 + sqrt( (c2 * c2) - (4*c3*c1)))/(2*c3);
    float root_2 = (-c2 - sqrt( (c2 * c2) - (4*c3*c1)))/(2*c3);
    float z2, z3;
    if (root_1 <= root_2) {
        z2 = root_1;
        z3 = root_2;
    }
    else {
        z3 = root_1;
        z2 = root_2;
    }


    //Calculating G
    float G;
    if (pixel_depth <= z2)
        G = 0.0f;
    else{
        if (pixel_depth <= z3){
            G = ((pixel_depth*z3) - (b_prime[0]*(pixel_depth + z3)) + b_prime[1])/
                ((z3 - z2)*(pixel_depth - z2));
        }
        else{
            G = 1 - (((z2*z3) - (b_prime[0]*(z2 + z3)) + b_prime[1]) / 
                        ((pixel_depth - z2)*(pixel_depth - z3)));
        }
    }

    //Convert colors into linear color space for calculations
    Kd = pow(Kd, vec3(2.2));
    Ks = pow(Ks, vec3(2.2));

    vec3 N = normalize(normalVec);
    vec3 L = normalize(lightPos - worldPos.xyz);
    vec3 V = normalize(eyePos - worldPos.xyz);
    vec3 H = normalize(L+V);

    float LN = max(dot(L, N), 0.0);
    float NH = max(dot(N, H), 0.0);
    float LH = max(dot(L, H), 0.0);

    if (lightingMode == Phong_M){
        alpha = -2 + (2/(shininess*shininess));
        outColor = ambient*Kd + (light*(Kd/PI)*LN + light*(Ks*10)*pow(NH, alpha)) * (1 - G);
        //Ks value coming in is for BRDF so adjust for Phong by multiplying by 10
    }
    else if (lightingMode == IBL_M){
        vec2 uv = vec2(-atan(-N.y, -N.x)/(2*PI), acos(-N.z)/PI);
        vec3 irr_map_color = texture2D(IrrMapTex, uv).xyz;
        irr_map_color = pow(irr_map_color, vec3(2.2));

        vec3 R = (2*dot(N, V)*N) - V;
        vec3 newH = normalize(R+V);
        uv = vec2(-atan(-R.y, -R.x)/(2*PI), acos(-R.z)/PI);
        vec3 reflection_map_color = texture2D(SkydomeTex, uv).xyz;
        reflection_map_color = pow(reflection_map_color, vec3(2.2));

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
      
        outColor = (Kd/PI)*irr_map_color + (reflection_map_color * brdf * NR);
        //outColor = (Kd/PI)*irr_map_color;
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

        /*
        if (pixel_depth > (light_depth + 0.01))
            outColor = ambient*Kd;
        else{*/
            outColor = ambient*Kd + ((light*LN*brdf) * (1-G));
            //outColor = ambient*Kd + ((light*LN*brdf) * S);
        //}
    }

    //Convert color back into sRGB space
    outColor = (exposure*outColor)/(exposure*outColor + 1);
    outColor = pow(outColor, vec3(1/2.2));
    
    
    if (reflective){
        vec3 reflectionColor;
        vec3 N = normalize(normalVec);
        vec3 V = normalize(eyePos - worldPos.xyz);
        vec3 R = (2*dot(N, V)*N) - V;
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

        if (reflectionMode == 0) //Mirror-like
            FragColor.xyz = reflectionColor;
        //Color mixed with brdf
        else if (reflectionMode == 1){
            vec3 H = normalize(R+V);
            float RH = max(dot(R, H), 0.0);
            float NH = max(dot(N, H), 0.0);
            float NR = max(dot(N, R), 0.0);
            vec3 brdf;
            float distribution;
        
            vec3 fresnel = Ks + ((vec3(1,1,1) - Ks)*pow((1-RH), 5));
            float visibility = 1/(RH*RH);
            float alpha = -2 + (2/(shininess*shininess));
            distribution = ((alpha+2)/(2*PI))*pow(NH, alpha);

            brdf = (Kd/PI) + ((fresnel*visibility*distribution)/4);
            FragColor.xyz = outColor + reflectionColor*NR*brdf;
        }
        else if (reflectionMode == 2) //Color mixed simple
            FragColor.xyz = outColor + reflectionColor*0.1;
        else
            FragColor.xyz = outColor;
    }
    else
        FragColor.xyz = outColor;
}
