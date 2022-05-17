/////////////////////////////////////////////////////////////////////////
// Pixel shader for Final output
////////////////////////////////////////////////////////////////////////
#version 330

const float PI = 3.14159f;

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
uniform sampler2D AOMap;
uniform sampler2D AOMap_1;
uniform sampler2D AOMap_2;
uniform int skydome_width;
uniform int skydome_height;
uniform int reflectionMode;
uniform int textureMode;
uniform int lightingMode;
uniform int drawFbo;
uniform float shininess;
uniform int width, height;
uniform float min_depth, max_depth;
uniform int ao_enabled;

uniform float bloomThreshold;

uniform HammersleyBlock {
    float sampling_count;
    float hammersley[2*100];
};

layout(location = 0) out vec4 RenderBuffer;
layout(location = 1) out vec4 PostProcessBuffer;

void main()
{
    //Following lines of code all read in values from the gbuffer
    //=================================================================
    vec2 uv = gl_FragCoord.xy / vec2(width, height);
    vec4 worldPos = texture2D(gBufferWorldPos, uv);
    vec3 normalVec = texture2D(gBufferNormalVec, uv).xyz;
    vec3 Kd = texture2D(gBufferDiffuse, uv).xyz;
    vec3 ao_factor = texture(AOMap_2, uv).xyz;
    vec3 Ks = texture2D(gBufferSpecular, uv).xyz;
    float shininess = texture2D(gBufferSpecular, uv).w;
    bool reflective = true;
    if (texture2D(gBufferDiffuse, uv).w == 0)
        reflective = false;

    bool isSkyDome = false;
    if (texture2D(gBufferNormalVec, uv).w == 1){
        isSkyDome = true;
        FragColor.xyz = Kd;
        RenderBuffer = FragColor;

        float luminance = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	    if (luminance > bloomThreshold)
		    PostProcessBuffer = vec4(vec3(FragColor.rgb), 1.0f);
	    else
		    PostProcessBuffer = vec4(vec3(0.0f), 1.0f);

        return;
    }
        

    vec4 shadowCoord = ShadowMatrix*vec4(worldPos.xyz, 1.0);
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
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 1){
        float light_depth;
        light_depth = texture2D(shadowMap, uv).y;
        light_depth = light_depth;
        FragColor.x = light_depth;
        FragColor.y = FragColor.x;
        FragColor.z = FragColor.x;
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 2){
        float light_depth;
        light_depth = texture2D(shadowMap, uv).z;
        light_depth = light_depth;
        FragColor.x = light_depth;
        FragColor.y = FragColor.x;
        FragColor.z = FragColor.x;
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 3){
        float light_depth;
        light_depth = texture2D(shadowMap, uv).w;
        light_depth = light_depth;
        FragColor.x = light_depth;
        FragColor.y = FragColor.x;
        FragColor.z = FragColor.x;
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 4){
        FragColor.xyz = texture2D(upperReflectionMap, uv).xyz;
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 5){
        FragColor.xyz = texture2D(lowerReflectionMap, uv).xyz;
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 6){
        FragColor.xyz = texture2D(gBufferWorldPos, uv).xyz/100;
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 7){
        FragColor.xyz = abs(texture2D(gBufferNormalVec, uv).xyz);
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 8){
        FragColor.xyz = texture2D(gBufferDiffuse, uv).xyz;
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 9){
        FragColor.xyz = texture2D(gBufferSpecular, uv).xyz;
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 10){
        FragColor.xyz = texture2D(AOMap, uv).xyz;
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 11){
        FragColor.xyz = texture2D(AOMap_1, uv).xyz;
        RenderBuffer = FragColor;
        return;
    }
    if (drawFbo == 12){
        FragColor.xyz = texture2D(AOMap_2, uv).xyz;
        RenderBuffer = FragColor;
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
        
        //Ks value coming in is for BRDF so adjust for Phong by multiplying by 10
        vec3 ambient_val = ambient*Kd;
        if (ao_enabled == 1)
            ambient_val *= ao_factor;
        outColor = ambient_val + (light*(Kd/PI)*LN + light*(Ks*10)*pow(NH, alpha)) * (1 - G);
    }
    else if (lightingMode == IBL_M){
        //Diffuse portion
        vec2 uv = vec2(-atan(-N.y, -N.x)/(2*PI), acos(-N.z)/PI);
        
        vec3 irr_map_color = texture2D(IrrMapTex, uv).xyz;
        irr_map_color = pow(irr_map_color, vec3(2.2));

        vec3 R = (2*dot(N, V)*N) - V;
        float NV = normalize(dot(N, V));
        vec3 newH = normalize(R+V);
        float RH = max(dot(R, newH), 0);
        float NR = max(dot(N, R), 0.0);
        float newNH = max(dot(N, newH), 0);
        float NL;

        vec3 A = normalize(vec3(-R.y, R.x, 0));
        vec3 B = normalize(cross(R, A));
        vec3 L;
        vec3 omega_k;
        vec3 spec_light;
        vec3 spec_calc;
        float lod;

        float distribution;
        vec3 fresnel;
        float visibility;
        float alpha_squared;
        float g1_L;
        float g1_V;
        float tan_v_L;
        float tan_v_V;
        vec3 avg_light = vec3(0);

        for(int i=0; i < sampling_count; i++) {
            uv.x = hammersley[i*2];
            uv.y = hammersley[(i*2)+1];
            uv.y = atan((shininess * sqrt(uv.y)) / sqrt(1 - uv.y));
            uv.y = uv.y/PI;
            L = vec3(
                cos(2*PI*(0.5 - uv.x)) * sin(PI*uv.y),
                sin(2*PI*(0.5 - uv.x)) * sin(PI*uv.y),
                cos(PI*uv.y)
            );
            omega_k = normalize(L.x*A + L.y*B + L.z*R);
            //Using uv as texture coords here not the uv direction
            uv = vec2(-atan(-omega_k.y, -omega_k.x)/(2*PI), acos(-omega_k.z)/PI);

            alpha_squared = pow(shininess, 2);
            distribution = alpha_squared / (PI * pow(pow(NH, 2) * (alpha_squared - 1.0) + 1, 2));
            lod = (0.5 * log2((skydome_width*skydome_height) / sampling_count)) - (0.5*(log2(distribution)/4));
            spec_light = textureLod(SkydomeTex, uv, lod).xyz;
            //Gamma correction
            spec_light = pow(spec_light, vec3(2.2));

            H = normalize(omega_k+V);
            LH = max(dot(omega_k, H), 0);
            NL = max(dot(N, omega_k), 0.0);
            fresnel = Ks + ((vec3(1,1,1) - Ks)*pow((1-LH), 5));
            tan_v_L = sqrt(1.0 - (NL*NL))/(NL);
            tan_v_V = sqrt(1.0 - (NV*NV))/(NV);
            if (tan_v_L == 0)
                g1_L = 0;
            else
                g1_L = 2.0/(1.0 + sqrt(1.0+(alpha_squared+(tan_v_L*tan_v_L))));
            if (tan_v_V == 0)
                g1_V = 0;
            else
                g1_V = 2.0/(1.0 + sqrt(1.0+(alpha_squared+(tan_v_V*tan_v_V))));
            visibility = 1/(LH*LH);

            avg_light += ((visibility*fresnel*spec_light)/(4) * NL);
        }

        spec_calc = avg_light/sampling_count;
      
        vec3 ambient_val = (Kd/PI)*irr_map_color;
        outColor = ambient_val + spec_calc;
        if (ao_enabled == 1)
            outColor *= ao_factor;
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
            
            //outColor = ambient*Kd + ((light*LN*brdf) * S);
        //}
        vec3 ambient_val = ambient*Kd;
        if (ao_enabled == 1)
            ambient_val *= ao_factor;

        outColor = ambient_val + ((light*LN*brdf) * (1-G));
    }
    
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
    
    RenderBuffer = FragColor;

    float luminance = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	if (luminance > bloomThreshold)
		PostProcessBuffer = vec4(vec3(max(FragColor.r, 0), 
                                      max(FragColor.g, 0), 
                                      max(FragColor.b, 0)), 1.0f);
	else{
        PostProcessBuffer = vec4(vec3(0.0f), 1.0f);
    }
}